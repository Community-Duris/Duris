#include "flatfile/flatfile_native_mobile_birth_ordinary_baseline_history.h"
#include <set>
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_ordinary_native_birth_receipt.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_reference_history.h"
#include "economy/native_mobile_birth_cash_role_result.h"
#include <type_traits>
#include <utility>
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

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
// The original name codec owns these objects and exact fresh requests. Its
// key grows 16 -> 64 while the old 16-byte allocation is still live; its
// inline prefix then reserve() requests the final 81 characters plus NUL.
std::string current_source_claim_name_bounded(const economic_operation_metadata &metadata,
					      flatfile_scratch_reserve_fn reserve, void *context,
					      size_t outer)
{
	require(metadata.source_event.has_value());
	const size_t first_key = metadata.lineage.bytes.size();
	const size_t complete_key = first_key + ECONOMIC_SOURCE_EVENT_BYTES;
	const size_t filename_size =
		sizeof("source-claim-") - 1 + SHA256_DIGEST_LENGTH * 2 + sizeof(".bin") - 1;
	static_assert(sizeof("source-claim-") - 1 <= 15);
	static_assert(sizeof("source-claim-") - 1 + SHA256_DIGEST_LENGTH * 2 + sizeof(".bin") - 1 >
		      30);
	size_t growth = first_key;
	require(receipt_bounded_add(growth, complete_key), status::capacity);
	size_t name_phase = complete_key;
	require(receipt_bounded_add(name_phase, filename_size + 1), status::capacity);
	size_t hash_phase = complete_key;
	require(receipt_bounded_add(hash_phase,
				    sizeof(std::span<const uint8_t>) + sizeof(economic_digest)),
		status::capacity);
	size_t working = sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>) +
			 sizeof(std::vector<uint8_t>) + sizeof(economic_digest) +
			 sizeof(std::string);
	require(receipt_bounded_add(
			working,
			std::max(
				std::max(growth, name_phase),
				std::max(hash_phase,
					 sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>)))),
		status::capacity);
	receipt_bounded_admit(outer, working, reserve, context);
	return source_claim_name(metadata);
}

struct current_source_claim_bytes_workspace
{
	economic_frozen_intent intent;
	std::span<const uint8_t> intent_wire;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> event{};
	std::vector<uint8_t> payload;
	explicit current_source_claim_bytes_workspace(const flatfile_accounting_record &record)
		: intent_wire(record.command.accounting_intent)
	{
	}
};
std::vector<uint8_t> current_source_claim_bytes_bounded(const flatfile_accounting_record &record,
							flatfile_scratch_reserve_fn reserve,
							void *context, size_t outer)
{
	const size_t fixed = sizeof(current_source_claim_bytes_workspace);
	receipt_bounded_admit(outer, fixed, reserve, context);
	current_source_claim_bytes_workspace work(record);
	checked(economic_intent_decode_bounded(work.intent_wire, &work.intent, reserve, context,
					       outer + fixed));
	require(work.intent.admission.metadata.source_event.has_value());
	size_t live = outer + fixed;
	require(receipt_bounded_add(live, work.intent.admission.facts.capacity()),
		status::capacity);
	receipt_bounded_admit(live, sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>),
			      reserve, context);
	checked(economic_source_event_encode(*work.intent.admission.metadata.source_event,
					     &work.event));
	// Same raw()/number() sequence and original growth, admitted before each
	// call. The raw span and old allocation survive the fresh request.
	receipt_bounded_admit(live,
			      work.intent.admission.metadata.lineage.bytes.size() +
				      sizeof(std::span<const uint8_t>),
			      reserve, context);
	raw(work.payload, work.intent.admission.metadata.lineage.bytes);
	size_t first_live = live;
	require(receipt_bounded_add(first_live, work.payload.capacity()), status::capacity);
	const size_t second_size = work.payload.size() + work.event.size();
	receipt_bounded_admit(first_live, second_size + sizeof(std::span<const uint8_t>), reserve,
			      context);
	raw(work.payload, work.event);
	size_t second_live = live;
	require(receipt_bounded_add(second_live, work.payload.capacity()), status::capacity);
	const size_t third_capacity = work.payload.size() * 2;
	receipt_bounded_admit(second_live, third_capacity + sizeof(std::span<const uint8_t>),
			      reserve, context);
	raw(work.payload, record.command.operation_id.bytes);
	number(work.payload, 1, 1);
	number(work.payload, 0, 7);
	size_t payload_live = live;
	require(receipt_bounded_add(payload_live, work.payload.capacity()), status::capacity);
	// Original envelope owns its payload span, result vector, exact fresh
	// reserve and hash/raw span/digest temporaries with payload still retained.
	size_t envelope_working = sizeof(std::span<const uint8_t>) + sizeof(std::vector<uint8_t>);
	require(receipt_bounded_add(envelope_working, header_bytes + work.payload.size()) &&
			receipt_bounded_add(envelope_working,
					    sizeof(economic_digest) +
						    sizeof(std::span<const uint8_t>)),
		status::capacity);
	receipt_bounded_admit(payload_live, envelope_working, reserve, context);
	return envelope(source_claim_magic, work.payload);
}

struct current_source_claim_workspace
{
	economic_frozen_intent intent;
	std::span<const uint8_t> intent_wire;
	std::vector<uint8_t> retained, expected;
	std::string directory, filename;
	explicit current_source_claim_workspace(const flatfile_accounting_record &record)
		: intent_wire(record.command.accounting_intent)
	{
	}
};
#endif
}

flatfile_accounting_status flatfile_accounting_storage::verify_source_claim_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_record &record, flatfile_scratch_reserve_fn reserve,
	void *context, size_t outer) noexcept
{
	if (!lock.owns(root) || !reserve)
		return status::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)record;
	(void)context;
	(void)outer;
	errno = ENOTSUP;
	return status::io_error;
#else
	try
	{
		const size_t fixed = sizeof(current_source_claim_workspace);
		receipt_bounded_admit(outer, fixed, reserve, context);
		current_source_claim_workspace work(record);
		checked(economic_intent_decode_bounded(work.intent_wire, &work.intent, reserve,
						       context, outer + fixed));
		if (!work.intent.admission.metadata.source_event)
			return status::ok;
		size_t live = outer + fixed;
		require(receipt_bounded_add(live, work.intent.admission.facts.capacity()),
			status::capacity);
		work.filename = current_source_claim_name_bounded(work.intent.admission.metadata,
								  reserve, context, live);
		require(work.filename.capacity() != SIZE_MAX &&
				receipt_bounded_add(live, work.filename.capacity() + 1),
			status::capacity);
		size_t directory_size = root.size();
		require(receipt_bounded_add(directory_size, sizeof("/economic-evidence") - 1),
			status::capacity);
		size_t path_peak = live;
		require(directory_size <= 15 ||
				(directory_size != SIZE_MAX &&
				 receipt_bounded_add(path_peak, directory_size + 1)),
			status::capacity);
		// Direct fresh length construction avoids root+suffix growth while
		// preserving the exact original directory bytes and read ordering.
		receipt_bounded_admit(path_peak, sizeof(std::string), reserve, context);
		{
			std::string directory(directory_size, '\0');
			std::copy(root.begin(), root.end(), directory.begin());
			std::copy_n("/economic-evidence", sizeof("/economic-evidence") - 1,
				    directory.begin() + root.size());
			work.directory = std::move(directory);
		}
		live = path_peak;
		const auto read_result = flatfile_read_bounded(
			work.directory, work.filename, 256, &work.retained, reserve, context, live);
		// Same original read() helper refuses IO/invalid before the receipt
		// result branches; budget refusal never becomes missing claim proof.
		require(read_result == flatfile_read_result::ok ||
				read_result == flatfile_read_result::not_found,
			read_result == flatfile_read_result::io_error ? status::io_error :
									status::invalid);
		if (record.result_code)
		{
			require(read_result == flatfile_read_result::not_found,
				read_result == flatfile_read_result::io_error ? status::io_error :
				read_result == flatfile_read_result::invalid  ? status::invalid :
										status::conflict);
			return status::ok;
		}
		require(read_result == flatfile_read_result::ok,
			read_result == flatfile_read_result::io_error ? status::io_error :
			read_result == flatfile_read_result::invalid  ? status::invalid :
									status::not_found);
		require(receipt_bounded_add(live, work.retained.capacity()), status::capacity);
		work.expected = current_source_claim_bytes_bounded(record, reserve, context, live);
		require(work.retained == work.expected, status::conflict);
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

// The exact typed future ordinary owner must separately prove its genuine
// original terminal carrier/native publication cut before sealing any origin.
flatfile_accounting_status
flatfile_ordinary_native_birth_receipt_storage::verify_retained_current_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation, flatfile_accounting_record *output,
	std::string *error)
{
	return guarded(
		[&]
		{
			require(output && !root.empty() && lock.matches(root) &&
				!critical_operation_id_is_zero(operation));
			// Full ORIGINAL index/active/sealed segment/stale-next/canonical record
			// proof, with recovery deliberately left at the caller's original cut.
			const auto current = load_context(root, bucket_for(operation), error);
			auto record = retained_in(root, current, operation, error);
			require(record.command.schema_version ==
					CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
				record.command.type == critical_command_type::native_mobile_birth &&
				record.command.payload_version ==
					NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
				!record.result_code &&
				record.failure_stage == critical_failure_stage::none &&
				record.durable_revision == 1 &&
				record.result.size() == NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES);
			quest_mobile_native_image born;
			std::vector<native_mobile_birth_item_recipe> recipes;
			native_mobile_birth_cash_role_recipe role;
			checked(native_mobile_birth_cash_role_command_decode(record.command, &born,
									     &recipes, &role));
			require(role.role == native_mobile_birth_cash_role::ordinary_wallet &&
				born.cash &&
				born.reference.birth_operation.bytes == operation.bytes);
			economic_frozen_intent intent;
			checked(economic_intent_decode(record.command.accounting_intent, &intent));
			checked(economic_intent_verify_binding(record.command, intent));
			require(intent.admission.metadata.source_event.has_value());
			native_mobile_birth_cash_role_result receipt;
			require(native_mobile_birth_cash_role_result_decode(record.result,
									    &receipt) &&
				receipt.role == native_mobile_birth_cash_role::ordinary_wallet);
			const economic_account_key wallet{ intent.admission.metadata.lineage,
							   economic_account_kind::wallet,
							   receipt.wallet_mapping_id,
							   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
			economic_accounting_plan expected;
			checked(native_mobile_birth_cash_role_accounting_compile(
				record.command, wallet, &expected));
			std::vector<uint8_t> plan_bytes;
			checked(economic_plan_encode(expected, &plan_bytes));
			require(plan_bytes == record.plan && expected.children.empty());
			native_mobile_birth_cash_role_result expected_receipt;
			checked(native_mobile_birth_cash_role_result_build(
				record.command, wallet, expected, &expected_receipt));
			std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES>
				receipt_bytes{};
			require(native_mobile_birth_cash_role_result_encode(expected_receipt,
									    &receipt_bytes) &&
				std::equal(receipt_bytes.begin(), receipt_bytes.end(),
					   record.result.begin()));

			// Historical mapping identity survives retirement. This genuine private
			// companion authenticates catalog/control/epoch/full mapping census;
			// it neither recovers nor uses today's active epoch/native balances.
			const unsigned int authority_error =
				flatfile_ordinary_native_birth_history_storage::verify_locked(
					root, lock, wallet, intent.admission.metadata.epoch,
					operation, born.reference.mobile_instance_id, error);
			require(!authority_error,
				authority_error == ENOMEM || authority_error == ENOSPC ?
					status::capacity :
				authority_error == EIO ? status::io_error :
							 status::invalid);
			std::vector<economic_accounting_item_reference> references;
			references.reserve(expected.item_events.size());
			for (const auto &event : expected.item_events)
			{
				require(event.event_index < UINT16_MAX);
				economic_accounting_item_reference reference;
				reference.operation_id = operation;
				reference.line_index = static_cast<uint16_t>(event.event_index);
				reference.event_index = event.event_index;
				reference.child_index = event.child_index;
				reference.item_uid = event.uid;
				reference.before_revision = event.before.revision;
				reference.after_revision = event.after.revision;
				reference.legacy_operation_id = operation;
				reference.legacy_event_index = reference.line_index;
				require(economic_accounting_item_reference_validate(reference));
				references.push_back(reference);
			}
			// The original passive full-set reader does not recover/repair/append;
			// this owner holds the SAME root lock around the entire synchronous read.
			flatfile_native_mobile_birth_ordinary_reference_current reference_counts;
			const auto references_error =
				flatfile_native_mobile_birth_ordinary_reference_history_storage::
					verify_current_operation_locked(root, lock, operation,
									references,
									&reference_counts, error);
			require(references_error == flatfile_item_accounting_status::ok,
				references_error == flatfile_item_accounting_status::capacity ?
					status::capacity :
				references_error == flatfile_item_accounting_status::io_error ?
					status::io_error :
				references_error == flatfile_item_accounting_status::not_found ?
					status::not_found :
					status::invalid);
			// Original exact source-claim bytes include lineage/event/original ID;
			// successful ordinary birth is required to have a source, never skipped.
			const auto claim_error = flatfile_accounting_storage::verify_source_claim(
				root, lock, record, error);
			require(claim_error == status::ok, claim_error);
			require(lock.matches(root));
			static_assert(
				std::is_nothrow_move_assignable_v<flatfile_accounting_record>);
			*output = std::move(record);
		},
		error);
}

namespace
{
// The original SCL1 payload is lineage16/event48/operation16/outcome1/padding7.
constexpr size_t ordinary_claim_bytes = header_bytes + 16 + ECONOMIC_SOURCE_EVENT_BYTES + 16 + 8;
struct ordinary_claim_directory
{
	DIR *value = nullptr;
	explicit ordinary_claim_directory(DIR *directory)
		: value(directory)
	{
	}
	ordinary_claim_directory(const ordinary_claim_directory &) = delete;
	ordinary_claim_directory &operator=(const ordinary_claim_directory &) = delete;
	~ordinary_claim_directory()
	{
		if (value)
			closedir(value);
	}
};
struct ordinary_claim_file
{
	int value = -1;
	explicit ordinary_claim_file(int descriptor)
		: value(descriptor)
	{
	}
	ordinary_claim_file(const ordinary_claim_file &) = delete;
	ordinary_claim_file &operator=(const ordinary_claim_file &) = delete;
	~ordinary_claim_file()
	{
		if (value >= 0)
			close(value);
	}
};
bool ordinary_claim_filename(const std::string &name)
{
	constexpr size_t prefix = sizeof("source-claim-") - 1;
	if (name.size() != prefix + 64 + 4 || name.compare(0, prefix, "source-claim-") ||
	    name.compare(prefix + 64, 4, ".bin"))
		return false;
	for (size_t index = prefix; index < prefix + 64; ++index)
		if (!((name[index] >= '0' && name[index] <= '9') ||
		      (name[index] >= 'a' && name[index] <= 'f')))
			return false;
	return true;
}
std::array<uint8_t, ordinary_claim_bytes> ordinary_claim_read(int parent, const std::string &name)
{
	ordinary_claim_file file(
		openat(parent, name.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW));
	require(file.value >= 0, errno == ELOOP ? status::invalid : status::io_error);
	struct stat info
	{
	};
	require(fstat(file.value, &info) == 0, status::io_error);
	require(S_ISREG(info.st_mode) && info.st_nlink == 1 && info.st_uid == geteuid() &&
		!(info.st_mode & 0077) && info.st_size == static_cast<off_t>(ordinary_claim_bytes));
	std::array<uint8_t, ordinary_claim_bytes> bytes{};
	size_t offset = 0;
	while (offset < bytes.size())
	{
		const ssize_t received =
			::read(file.value, bytes.data() + offset, bytes.size() - offset);
		if (received < 0 && errno == EINTR)
			continue;
		require(received > 0, status::io_error);
		offset += static_cast<size_t>(received);
	}
	uint8_t trailing = 0;
	ssize_t received;
	do
	{
		received = ::read(file.value, &trailing, 1);
	} while (received < 0 && errno == EINTR);
	require(received >= 0, status::io_error);
	require(!received);
	struct stat after
	{
	};
	require(fstat(file.value, &after) == 0, status::io_error);
	require(after.st_dev == info.st_dev && after.st_ino == info.st_ino &&
		after.st_mode == info.st_mode && after.st_uid == info.st_uid &&
		after.st_nlink == info.st_nlink && after.st_size == info.st_size);
	const int descriptor = file.value;
	file.value = -1;
	require(close(descriptor) == 0, status::io_error);
	return bytes;
}
void ordinary_birth_claim_census_locked(const std::string &root,
					const flatfile_authority_lock &lock,
					const critical_command &command, bool retained,
					std::string *error)
{
	require(!root.empty() && lock.matches(root) &&
		command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		command.type == critical_command_type::native_mobile_birth &&
		command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
		critical_command_envelope_valid(command) &&
		!critical_operation_id_is_zero(command.operation_id));
	quest_mobile_native_image born;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	checked(native_mobile_birth_cash_role_command_decode(command, &born, &recipes, &role));
	require(role.role == native_mobile_birth_cash_role::ordinary_wallet && born.cash &&
		born.reference.birth_operation.bytes == command.operation_id.bytes);
	economic_frozen_intent intent;
	checked(economic_intent_decode(command.accounting_intent, &intent));
	checked(economic_intent_verify_binding(command, intent));
	require(intent.admission.metadata.source_event.has_value());
	const auto &metadata = intent.admission.metadata;
	const std::string selected = source_claim_name(metadata);
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> event{};
	checked(economic_source_event_encode(*metadata.source_event, &event));
	std::vector<uint8_t> expected_payload;
	raw(expected_payload, metadata.lineage.bytes);
	raw(expected_payload, event);
	raw(expected_payload, command.operation_id.bytes);
	number(expected_payload, 1, 1);
	number(expected_payload, 0, 7);
	const auto expected = envelope(source_claim_magic, expected_payload);
	require(expected.size() == ordinary_claim_bytes);

	ordinary_claim_file directory_fd(
		open(directory(root).c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW));
	require(directory_fd.value >= 0, errno == ELOOP ? status::invalid : status::io_error);
	struct stat directory_info
	{
	};
	require(fstat(directory_fd.value, &directory_info) == 0, status::io_error);
	require(S_ISDIR(directory_info.st_mode) && directory_info.st_uid == geteuid() &&
		!(directory_info.st_mode & 0077));
	ordinary_claim_directory entries(fdopendir(directory_fd.value));
	require(entries.value, status::io_error);
	directory_fd.value = -1; // fdopendir now owns this exact secure directory FD.
	const int parent = dirfd(entries.value);
	require(parent >= 0, status::io_error);
	size_t operation_claims = 0;
	bool selected_found = false;
	for (;;)
	{
		// Each read's errno is isolated from every intervening codec/I/O call.
		errno = 0;
		const auto *entry = readdir(entries.value);
		if (!entry)
		{
			require(!errno, status::io_error);
			break;
		}
		const std::string name(entry->d_name);
		if (name.compare(0, sizeof("source-claim-") - 1, "source-claim-"))
			continue; // Other original evidence namespaces and atomic dot temporaries.
		require(ordinary_claim_filename(name));
		const auto bytes = ordinary_claim_read(parent, name);
		reader input{ unwrap(bytes, source_claim_magic, ordinary_claim_bytes) };
		economic_operation_metadata actual;
		const auto lineage = input.take(actual.lineage.bytes.size());
		std::copy(lineage.begin(), lineage.end(), actual.lineage.bytes.begin());
		require(!critical_operation_id_is_zero(actual.lineage));
		const auto encoded_event = input.take(ECONOMIC_SOURCE_EVENT_BYTES);
		economic_source_event decoded_event;
		checked(economic_source_event_decode(encoded_event, &decoded_event));
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> canonical_event{};
		checked(economic_source_event_encode(decoded_event, &canonical_event));
		require(std::equal(canonical_event.begin(), canonical_event.end(),
				   encoded_event.begin()));
		actual.source_event = decoded_event;
		critical_operation_id operation;
		const auto encoded_operation = input.take(operation.bytes.size());
		std::copy(encoded_operation.begin(), encoded_operation.end(),
			  operation.bytes.begin());
		require(!critical_operation_id_is_zero(operation));
		require(input.number(1) == 1 && input.number(7) == 0);
		input.done();
		require(name == source_claim_name(actual));
		if (operation.bytes == command.operation_id.bytes)
		{
			++operation_claims;
			require(retained && operation_claims == 1, status::conflict);
		}
		if (name == selected)
		{
			require(retained && !selected_found &&
					std::equal(bytes.begin(), bytes.end(), expected.begin()),
				status::conflict);
			selected_found = true;
		}
	}
	DIR *completed = entries.value;
	entries.value = nullptr;
	require(closedir(completed) == 0, status::io_error);
	require(lock.matches(root));
	require(operation_claims == (retained ? 1U : 0U) && selected_found == retained,
		retained && !selected_found ? status::not_found : status::conflict);
	(void)error; // Diagnostics remain at the original guarded public boundary.
}
}

flatfile_accounting_status
flatfile_ordinary_native_birth_receipt_storage::verify_source_claim_absent_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, std::string *error)
{
	return guarded([&]
		       { ordinary_birth_claim_census_locked(root, lock, command, false, error); },
		       error);
}

flatfile_accounting_status
flatfile_ordinary_native_birth_receipt_storage::verify_source_claim_current_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_record &record, std::string *error)
{
	return guarded(
		[&]
		{
			require(!record.result_code &&
				record.failure_stage == critical_failure_stage::none &&
				record.durable_revision == 1 &&
				record.result.size() == NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES);
			ordinary_birth_claim_census_locked(root, lock, record.command, true, error);
		},
		error);
}

namespace
{
void ordinary_history_authority_checked(unsigned int error)
{
	require(!error, error == ENOMEM || error == ENOSPC || error == EOVERFLOW ?
				status::capacity :
			error == EIO ? status::io_error :
				       status::invalid);
}
std::vector<uint64_t> ordinary_history_born_uids(const quest_mobile_native_image &image)
{
	std::vector<uint64_t> born;
	born.reserve(image.items.size());
	for (const auto &literal : image.items)
		born.push_back(literal.object_uid);
	std::sort(born.begin(), born.end());
	require(std::adjacent_find(born.begin(), born.end()) == born.end() &&
		(born.empty() || born.front()));
	return born;
}

// Only the closed receipt-owner entries below create metadata with the owning
// passive storage reader. This helper has no public DTO/capability entry point.
template <class Visit> void ordinary_history_scan_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_native_mobile_birth_ordinary_retained_metadata &metadata,
	flatfile_ordinary_native_birth_economic_history_counts &observed, Visit visit,
	std::string *error)
{
	require(!root.empty() && lock.matches(root));
	std::set<std::array<uint8_t, 16>> retained_epochs;
	for (const auto &epoch : metadata.epochs)
		require(retained_epochs.insert(epoch.epoch.bytes).second);
	require(metadata.epochs.size() == metadata.control.epoch_count);
	observed.lineage = metadata.control.lineage;
	observed.lineage_revision = metadata.control.revision;
	observed.epochs_digest = metadata.control.epochs_digest;
	observed.retained_epochs = static_cast<uint32_t>(metadata.epochs.size());
	for (size_t bucket = 0; bucket < FLATFILE_ACCOUNTING_BUCKETS; ++bucket)
	{
		const bool initialized = (metadata.control.evidence_initialized[bucket / 8] &
					  (uint8_t{ 1 } << (bucket % 8))) != 0;
		if (!initialized)
		{
			// Original full-prefix namespace fence. File absence alone does
			// not authenticate a clear initialized bit or an empty bucket.
			require_empty_bucket(root, bucket);
			++observed.clear_buckets;
		}
		else
		{
			auto value = load_context(root, bucket, error);
			require(value.index.lineage.bytes == metadata.control.lineage.bytes,
				status::conflict);
			++observed.initialized_buckets;
			if (!value.index.entries.empty())
			{
				const uint32_t active = last_segment(value.index);
				for (uint32_t segment = 0; segment <= active; ++segment)
				{
					// load_context already authenticates the active segment
					// and stale-next fence. Each sealed segment is fully read
					// with the original checksum/count/index digest decoder.
					const auto bytes = segment == active ?
								   std::move(value.active) :
								   load_segment(root, value.index,
										segment, error);
					for (const auto *item :
					     segment_entries(value.index, segment))
					{
						const auto wire =
							std::span<const uint8_t>(bytes).subspan(
								header_bytes + 32 + item->offset,
								item->bytes);
						auto record = decode_record(wire);
						economic_frozen_intent intent;
						checked(economic_intent_decode(
							record.command.accounting_intent, &intent));
						checked(economic_intent_verify_binding(
							record.command, intent));
						require(record.command.operation_id.bytes ==
								item->id.bytes &&
							intent.admission.metadata.lineage.bytes ==
								value.index.lineage.bytes &&
							retained_epochs.contains(
								intent.admission.metadata.epoch
									.bytes));
						// Full record round trip includes every original command,
						// outcome/revision/plan/result byte, not a projected DTO.
						const auto canonical = encode_record(record);
						require(canonical.size() == wire.size() &&
							std::equal(canonical.begin(),
								   canonical.end(), wire.begin()));
						++observed.records_verified;
						if (record.result_code)
						{
							++observed.failed_records;
							visit(record, nullptr, wire);
						}
						else
						{
							economic_accounting_plan plan;
							checked(economic_plan_decode(record.plan,
										     &plan));
							require(plan.metadata.operation_id.bytes ==
									record.command.operation_id
										.bytes &&
								plan.metadata.lineage.bytes ==
									metadata.control.lineage
										.bytes &&
								plan.metadata.epoch.bytes ==
									intent.admission.metadata
										.epoch.bytes &&
								plan.children.empty());
							++observed.successful_records;
							observed.item_events_verified +=
								plan.item_events.size();
							visit(record, &plan, wire);
						}
					}
					++observed.segments_verified;
				}
			}
		}
		++observed.buckets_verified;
	}
	require(observed.buckets_verified == FLATFILE_ACCOUNTING_BUCKETS && lock.matches(root));
}

void ordinary_history_verify_creation_events(const critical_command &command,
					     const quest_mobile_native_image &image,
					     const economic_accounting_plan &plan)
{
	require(command.type == critical_command_type::native_mobile_birth &&
		command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
		image.reference.birth_operation.bytes == command.operation_id.bytes &&
		plan.metadata.operation_id.bytes == command.operation_id.bytes && image.cash &&
		image.cash->revision == 1 && plan.children.empty() &&
		plan.item_events.size() == image.items.size() &&
		plan.items_before.size() == image.items.size() &&
		plan.items_after.size() == image.items.size());
	std::vector<uint64_t> roots;
	roots.reserve(image.items.size());
	for (size_t index = 0; index < image.items.size(); ++index)
	{
		const auto &literal = image.items[index];
		uint64_t root_uid = literal.object_uid, parent_uid = 0;
		if (literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		{
			require(literal.parent_index >= 0 &&
				static_cast<size_t>(literal.parent_index) < index &&
				!literal.equipment_slot);
			const size_t parent = static_cast<size_t>(literal.parent_index);
			root_uid = roots[parent];
			parent_uid = image.items[parent].object_uid;
		}
		roots.push_back(root_uid);
		const auto &event = plan.item_events[index];
		// Original SQL ledger's operation/event/UID, root/parent, from-owner
		// and to-owner/context, item revision and equipment fields. Complete
		// NMB4 ordinary compilation already binds the full EAP1 bytes.
		require(event.event_index == index && !event.child_index &&
			event.uid == literal.object_uid &&
			economic_item_position_equal(event.before, economic_item_position{}) &&
			event.after.owner.type == item_owner_type::native_mobile &&
			event.after.owner.id == image.reference.mobile_instance_id &&
			event.after.owner.context_id == 0 && event.after.root_uid == root_uid &&
			event.after.parent_uid == parent_uid && event.after.revision == 1 &&
			event.after.state == item_custody_state::active &&
			event.after.equipment_slot == literal.equipment_slot);
		// The typed creation family fixes from/to owner revisions 0/1 and
		// creation reason/id creation/0 in the original SQL ledger marshaller;
		// source_site is the actual immutable command field. No independent
		// mutable ledger DTO or fictitious disk columns supply those values.
		const auto prior = std::lower_bound(plan.items_before.begin(),
						    plan.items_before.end(), literal.object_uid,
						    [](const economic_item_snapshot &row,
						       uint64_t uid) { return row.uid < uid; });
		const auto after = std::lower_bound(plan.items_after.begin(),
						    plan.items_after.end(), literal.object_uid,
						    [](const economic_item_snapshot &row,
						       uint64_t uid) { return row.uid < uid; });
		require(prior != plan.items_before.end() && prior->uid == literal.object_uid &&
			economic_item_position_equal(prior->position, event.before) &&
			after != plan.items_after.end() && after->uid == literal.object_uid &&
			economic_item_position_equal(after->position, event.after));
	}
}
}

flatfile_accounting_status
flatfile_ordinary_native_birth_receipt_storage::verify_initial_history_absence_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_ordinary_native_birth_economic_history_counts *output, std::string *error)
{
	return guarded(
		[&]
		{
			require(output && !root.empty() && lock.matches(root) &&
				native_mobile_birth_cash_role_recovery_initial(original));
			quest_mobile_native_image image;
			std::vector<native_mobile_birth_item_recipe> recipes;
			native_mobile_birth_cash_role_recipe role;
			economic_frozen_intent intent;
			checked(native_mobile_birth_cash_role_command_decode(
				original.command, &image, &recipes, &role));
			checked(economic_intent_decode(original.command.accounting_intent,
						       &intent));
			checked(economic_intent_verify_binding(original.command, intent));
			require(role.role == native_mobile_birth_cash_role::ordinary_wallet &&
				image.cash &&
				image.reference.birth_operation.bytes ==
					original.command.operation_id.bytes &&
				intent.admission.metadata.source_event.has_value());
			const auto born = ordinary_history_born_uids(image);
			flatfile_native_mobile_birth_ordinary_retained_metadata metadata;
			ordinary_history_authority_checked(
				flatfile_native_mobile_birth_ordinary_baseline_history_storage::
					read_metadata_locked(root, lock, &metadata, error));
			require(metadata.control.lineage.bytes ==
				intent.admission.metadata.lineage.bytes);
			flatfile_native_mobile_birth_ordinary_baseline_absence baseline;
			const auto baseline_status =
				flatfile_native_mobile_birth_ordinary_baseline_history_storage::
					verify_initial_absence_locked(root, lock, original,
								      &baseline, error);
			require(baseline_status == status::ok, baseline_status);
			require(baseline.lineage.bytes == metadata.control.lineage.bytes &&
				baseline.lineage_revision == metadata.control.revision &&
				baseline.epochs_digest == metadata.control.epochs_digest &&
				baseline.born_uids == born.size());
			const auto claim_status = verify_source_claim_absent_locked(
				root, lock, original.command, error);
			require(claim_status == status::ok, claim_status);

			flatfile_ordinary_native_birth_economic_history_counts observed;
			observed.born_uids = born.size();
			observed.baseline_indexes_verified = baseline.indexes_verified;
			observed.baseline_item_reservations_verified =
				baseline.item_reservations_verified;
			ordinary_history_scan_locked(
				root, lock, metadata, observed,
				[&](const flatfile_accounting_record &record,
				    const economic_accounting_plan *plan, std::span<const uint8_t>)
				{
					// Any retained outcome already owns this operation ID.
					require(record.command.operation_id.bytes !=
							original.command.operation_id.bytes,
						status::already_exists);
					if (!plan)
						return;
					for (const auto &event : plan->item_events)
						require(!std::binary_search(born.begin(),
									    born.end(), event.uid),
							status::already_exists);
					if (record.command.type ==
					    critical_command_type::economic_baseline)
					{
						// Baseline has before==after witnesses and NO item
						// events. Its original UID history must not disappear.
						for (const auto *rows :
						     { &plan->items_before, &plan->items_after })
							for (const auto &row : *rows)
							{
								require(!std::binary_search(
										born.begin(),
										born.end(),
										row.uid),
									status::already_exists);
								++observed.baseline_witness_rows_verified;
							}
					}
				},
				error);
			require(lock.matches(root));
			static_assert(std::is_nothrow_copy_assignable_v<
				      flatfile_ordinary_native_birth_economic_history_counts>);
			*output = observed;
		},
		error);
}

flatfile_accounting_status
flatfile_ordinary_native_birth_receipt_storage::verify_retained_history_current_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation, flatfile_accounting_record *output,
	flatfile_ordinary_native_birth_economic_history_counts *counts, std::string *error)
{
	return guarded(
		[&]
		{
			require(output && counts && !root.empty() && lock.matches(root) &&
				!critical_operation_id_is_zero(operation));
			flatfile_accounting_record record;
			const auto receipt_status = verify_retained_current_locked(
				root, lock, operation, &record, error);
			require(receipt_status == status::ok, receipt_status);
			const auto claim_status =
				verify_source_claim_current_locked(root, lock, record, error);
			require(claim_status == status::ok, claim_status);

			quest_mobile_native_image image;
			std::vector<native_mobile_birth_item_recipe> recipes;
			native_mobile_birth_cash_role_recipe role;
			checked(native_mobile_birth_cash_role_command_decode(record.command, &image,
									     &recipes, &role));
			require(role.role == native_mobile_birth_cash_role::ordinary_wallet);
			native_mobile_birth_cash_role_result receipt;
			require(native_mobile_birth_cash_role_result_decode(record.result,
									    &receipt) &&
				receipt.role == native_mobile_birth_cash_role::ordinary_wallet);
			economic_frozen_intent intent;
			checked(economic_intent_decode(record.command.accounting_intent, &intent));
			checked(economic_intent_verify_binding(record.command, intent));
			const economic_account_key wallet{ intent.admission.metadata.lineage,
							   economic_account_kind::wallet,
							   receipt.wallet_mapping_id,
							   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
			economic_accounting_plan expected;
			checked(native_mobile_birth_cash_role_accounting_compile(
				record.command, wallet, &expected));
			std::vector<uint8_t> expected_plan;
			checked(economic_plan_encode(expected, &expected_plan));
			require(expected_plan == record.plan);
			ordinary_history_verify_creation_events(record.command, image, expected);
			const auto original_record = encode_record(record);
			const auto born = ordinary_history_born_uids(image);
			flatfile_native_mobile_birth_ordinary_retained_metadata metadata;
			ordinary_history_authority_checked(
				flatfile_native_mobile_birth_ordinary_baseline_history_storage::
					read_metadata_locked(root, lock, &metadata, error));
			require(metadata.control.lineage.bytes == expected.metadata.lineage.bytes);

			flatfile_ordinary_native_birth_economic_history_counts observed;
			observed.born_uids = born.size();
			ordinary_history_scan_locked(
				root, lock, metadata, observed,
				[&](const flatfile_accounting_record &retained,
				    const economic_accounting_plan *plan,
				    std::span<const uint8_t> wire)
				{
					if (retained.command.operation_id.bytes == operation.bytes)
					{
						require(plan &&
							original_record.size() == wire.size() &&
							std::equal(original_record.begin(),
								   original_record.end(),
								   wire.begin()));
						ordinary_history_verify_creation_events(
							retained.command, image, *plan);
						require(retained.command.source_site ==
								record.command.source_site &&
							retained.plan == expected_plan &&
							plan->accounts.size() ==
								expected.accounts.size() &&
							plan->postings.size() ==
								expected.postings.size() &&
							plan->children.empty());
						++observed.birth_records_verified;
						observed.creation_events_verified +=
							plan->item_events.size();
						observed.birth_account_effects_verified +=
							plan->accounts.size();
						observed.birth_coin_postings_verified +=
							plan->postings.size();
						return;
					}
					if (!plan)
						return;
					for (const auto &event : plan->item_events)
						// Original SQL ledger UNIQUE(item_uid,item_revision).
						// Do NOT apply this key to references or to baseline
						// before==after revision1 witnesses that have no event.
						require(event.after.revision != 1 ||
								!std::binary_search(born.begin(),
										    born.end(),
										    event.uid),
							status::conflict);
					if (retained.command.type ==
					    critical_command_type::economic_baseline)
						observed.baseline_witness_rows_verified +=
							plan->items_before.size() +
							plan->items_after.size();
				},
				error);
			require(observed.birth_records_verified == 1 &&
				observed.creation_events_verified == expected.item_events.size() &&
				observed.birth_account_effects_verified ==
					expected.accounts.size() &&
				observed.birth_coin_postings_verified == expected.postings.size() &&
				lock.matches(root));
			static_assert(
				std::is_nothrow_move_assignable_v<flatfile_accounting_record>);
			static_assert(std::is_nothrow_copy_assignable_v<
				      flatfile_ordinary_native_birth_economic_history_counts>);
			// Both final transfers are nonthrowing; no fallible tail can
			// publish only one output after an earlier refusal.
			*output = std::move(record);
			*counts = observed;
		},
		error);
}

flatfile_accounting_status flatfile_accounting_storage::stage_ordinary_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_record &record,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
{
	return guarded(
		[&]
		{
			validate_append(operations, 2);
			require(critical_command_envelope_valid(record.command));
			require(!root.empty() && lock.matches(root));
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

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
// Only explicit source objects and pinned libstdc++ requests are counted here.
// Library, system, OpenSSL and emitted/native frame qualification remain separate.
struct ordinary_receipt_forward
{
	flatfile_scratch_reserve_fn reserve;
	void *context;
	bool refused = false;
	static bool admit(size_t absolute, void *opaque) noexcept
	{
		auto &value = *static_cast<ordinary_receipt_forward *>(opaque);
		const size_t relay = sizeof(size_t) * 2 + sizeof(void *) * 2 + sizeof(bool);
		if (value.refused || !receipt_bounded_add(absolute, relay) ||
		    !value.reserve(absolute, value.context))
		{
			value.refused = true;
			return false;
		}
		return true;
	}
};
// Existing secure-read companions own the stat/vector/data objects. Their
// named parameter/scalar/private-directory/read_all frames remain a caller
// responsibility. Keep these real captured source frames through every nested
// read; this does not assert compiler/system or standard-library stack sizes.
constexpr size_t ordinary_receipt_secure_read_source_frames =
	5 * sizeof(void *) + sizeof(flatfile_scratch_reserve_fn) + 4 * sizeof(size_t) +
	5 * sizeof(int) + sizeof(bool) +
	// read_all(fd,data,size): fd, data, size, chunk and received.
	sizeof(int) + sizeof(void *) + 2 * sizeof(size_t) + sizeof(ssize_t) + sizeof(bool);
// Nonallocating canonical MBR4 decoder/encoder/validator plus put/get source
// frames. The caller retains its inline destination separately. The successful
// decoder owns both its candidate and canonical array while the encoder runs.
constexpr size_t ordinary_receipt_result_source_frames =
	sizeof(std::span<const uint8_t>) + sizeof(void *) +
	sizeof(native_mobile_birth_cash_role_result) +
	2 * sizeof(std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES>) +
	11 * sizeof(uint64_t) + 4 * sizeof(size_t) + 6 * sizeof(void *) + 3 * sizeof(uint64_t) +
	3 * sizeof(bool);
size_t ordinary_receipt_sum(size_t first, size_t second)
{
	require(receipt_bounded_add(first, second), status::capacity);
	return first;
}
template <class T> size_t ordinary_receipt_vector_heap(const std::vector<T> &value)
{
	require(value.capacity() <= SIZE_MAX / sizeof(T), status::capacity);
	return value.capacity() * sizeof(T);
}
size_t ordinary_receipt_string_heap(const std::string &value)
{
	// Inline bytes already belong to the containing string object.
	if (value.capacity() <= 15)
		return 0;
	require(value.capacity() != SIZE_MAX, status::capacity);
	return value.capacity() + 1;
}
void ordinary_receipt_plan_encode_bounded(const economic_accounting_plan &plan,
					  std::vector<uint8_t> *output,
					  flatfile_scratch_reserve_fn reserve, void *context,
					  size_t outer)
{
	const size_t frame = sizeof(economic_accounting_plan_allocation_profile) +
			     4 * sizeof(void *) + sizeof(flatfile_scratch_reserve_fn) +
			     3 * sizeof(size_t);
	receipt_bounded_admit(outer, frame, reserve, context);
	economic_accounting_plan_allocation_profile profile;
	const size_t live = ordinary_receipt_sum(outer, frame);
	receipt_bounded_admit(live, economic_plan_allocation_preflight_working_bytes(), reserve,
			      context);
	checked(economic_plan_allocation_preflight(plan, &profile));
	require(profile.storage_policy_supported, status::capacity);
	receipt_bounded_admit(live, profile.encode_working_bytes, reserve, context);
	checked(economic_plan_encode(plan, output));
}
struct ordinary_receipt_record_encoding_workspace
{
	std::vector<uint8_t> command, payload;
	size_t payload_size = 26, live = 0, envelope_request = 0;
};
std::vector<uint8_t>
ordinary_receipt_record_encode_bounded(const flatfile_accounting_record &record,
				       flatfile_scratch_reserve_fn reserve, void *context,
				       size_t outer)
{
	const size_t frame = sizeof(ordinary_receipt_record_encoding_workspace) +
			     2 * sizeof(void *) + sizeof(flatfile_scratch_reserve_fn) +
			     2 * sizeof(size_t);
	receipt_bounded_admit(outer, frame, reserve, context);
	ordinary_receipt_record_encoding_workspace work;
	work.live = ordinary_receipt_sum(outer, frame);
	work.command = receipt_validate_record_bounded(record, reserve, context, work.live);
	work.live = ordinary_receipt_sum(work.live, work.command.capacity());
	work.payload_size = ordinary_receipt_sum(work.payload_size, work.command.size());
	work.payload_size = ordinary_receipt_sum(work.payload_size, record.plan.size());
	work.payload_size = ordinary_receipt_sum(work.payload_size, record.result.size());
	// Exact original reserve before construction; no growth occurs afterwards.
	receipt_bounded_admit(work.live,
			      ordinary_receipt_sum(work.payload_size,
						   sizeof(std::span<const uint8_t>) +
							   2 * sizeof(uint64_t) + sizeof(size_t)),
			      reserve, context);
	work.payload.reserve(work.payload_size);
	number(work.payload, work.command.size(), 4);
	number(work.payload, record.plan.size(), 4);
	number(work.payload, record.result.size(), 4);
	number(work.payload, record.result_code, 4);
	number(work.payload, record.durable_revision, 8);
	number(work.payload, static_cast<uint16_t>(record.failure_stage), 2);
	raw(work.payload, work.command);
	raw(work.payload, record.plan);
	raw(work.payload, record.result);
	work.live = ordinary_receipt_sum(work.live, work.payload.capacity());
	work.envelope_request = ordinary_receipt_sum(header_bytes, work.payload.size());
	work.envelope_request = ordinary_receipt_sum(
		work.envelope_request, sizeof(std::vector<uint8_t>) + sizeof(economic_digest) +
					       3 * sizeof(std::span<const uint8_t>) +
					       3 * sizeof(void *) + 2 * sizeof(uint64_t) +
					       2 * sizeof(size_t));
	receipt_bounded_admit(work.live, work.envelope_request, reserve, context);
	return envelope(record_magic, work.payload);
}
std::string ordinary_receipt_directory_bounded(const std::string &root,
					       flatfile_scratch_reserve_fn reserve, void *context,
					       size_t outer)
{
	const size_t length = ordinary_receipt_sum(root.size(), sizeof("/economic-evidence") - 1);
	const size_t frame = sizeof(std::string) + 2 * sizeof(void *) +
			     sizeof(flatfile_scratch_reserve_fn) + 3 * sizeof(size_t);
	receipt_bounded_admit(
		outer,
		ordinary_receipt_sum(frame, length > 15 ? ordinary_receipt_sum(length, 1) : 0),
		reserve, context);
	std::string value(length, '\0');
	std::copy(root.begin(), root.end(), value.begin());
	std::copy_n("/economic-evidence", sizeof("/economic-evidence") - 1,
		    value.begin() + root.size());
	return value;
}
void ordinary_receipt_empty_bucket_bounded(const std::string &root, size_t bucket,
					   flatfile_scratch_reserve_fn reserve, void *context,
					   size_t outer)
{
	// Same whole-prefix namespace fence, secure FD checks and original errors.
	const size_t frame = 2 * sizeof(std::string) + sizeof(struct stat) + 2 * sizeof(void *) +
			     sizeof(flatfile_scratch_reserve_fn) + 3 * sizeof(size_t) +
			     3 * sizeof(int) + sizeof(DIR *) + sizeof(struct dirent *) +
			     2 * sizeof(bool);
	receipt_bounded_admit(outer, frame, reserve, context);
	const auto prefix = bucket_prefix(bucket); // nine-byte inline prefix.
	const auto path = ordinary_receipt_directory_bounded(root, reserve, context,
							     ordinary_receipt_sum(outer, frame));
	receipt_bounded_admit(ordinary_receipt_sum(outer, frame),
			      ordinary_receipt_string_heap(path), reserve, context);
	const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
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
std::vector<uint64_t> ordinary_receipt_born_bounded(const quest_mobile_native_image &image,
						    flatfile_scratch_reserve_fn reserve,
						    void *context, size_t outer)
{
	const size_t frame = sizeof(std::vector<uint64_t>) + 3 * sizeof(void *) +
			     sizeof(flatfile_scratch_reserve_fn) + 2 * sizeof(size_t);
	require(image.items.size() <= SIZE_MAX / sizeof(uint64_t), status::capacity);
	receipt_bounded_admit(outer,
			      ordinary_receipt_sum(frame, image.items.size() * sizeof(uint64_t)),
			      reserve, context);
	return ordinary_history_born_uids(image);
}
void ordinary_receipt_creation_bounded(const critical_command &command,
				       const quest_mobile_native_image &image,
				       const economic_accounting_plan &plan,
				       flatfile_scratch_reserve_fn reserve, void *context,
				       size_t outer)
{
	// The genuine original reserve(image.items.size()) and all named source
	// loop objects coexist with the caller's full plan/image/record storage.
	const size_t frame = sizeof(std::vector<uint64_t>) + 2 * sizeof(uint64_t) +
			     2 * sizeof(size_t) + 6 * sizeof(void *) +
			     2 * sizeof(decltype(plan.items_before.begin())) +
			     sizeof(economic_item_position) + sizeof(flatfile_scratch_reserve_fn) +
			     2 * sizeof(size_t);
	require(image.items.size() <= SIZE_MAX / sizeof(uint64_t), status::capacity);
	receipt_bounded_admit(outer,
			      ordinary_receipt_sum(frame, image.items.size() * sizeof(uint64_t)),
			      reserve, context);
	ordinary_history_verify_creation_events(command, image, plan);
}
struct ordinary_receipt_claim_workspace
{
	quest_mobile_native_image born;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_frozen_intent intent;
	std::span<const uint8_t> intent_wire;
	std::string selected, path;
	std::vector<uint8_t> expected;
	size_t image_heap = 0, recipe_heap = 0, live = 0, operation_claims = 0;
	bool selected_found = false;
};
void ordinary_receipt_claim_census_bounded(const std::string &root,
					   const flatfile_authority_lock &lock,
					   const flatfile_accounting_record &record,
					   flatfile_scratch_reserve_fn reserve, void *context,
					   size_t outer)
{
	const auto &command = record.command;
	require(!root.empty() && lock.matches(root) &&
		command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		command.type == critical_command_type::native_mobile_birth &&
		command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
		critical_command_envelope_valid(command) &&
		!critical_operation_id_is_zero(command.operation_id));
	const size_t frame = sizeof(ordinary_receipt_claim_workspace) +
			     sizeof(ordinary_claim_file) + sizeof(ordinary_claim_directory) +
			     sizeof(struct stat) + 5 * sizeof(void *) +
			     sizeof(flatfile_scratch_reserve_fn) + 2 * sizeof(size_t) +
			     sizeof(int) + sizeof(DIR *);
	receipt_bounded_admit(outer, frame, reserve, context);
	ordinary_receipt_claim_workspace work;
	work.live = ordinary_receipt_sum(outer, frame);
	checked(native_mobile_birth_cash_role_command_decode_bounded(
		command, &work.born, &work.recipes, &work.role, reserve, context, work.live,
		&work.image_heap, &work.recipe_heap));
	work.live = ordinary_receipt_sum(work.live, work.image_heap);
	work.live = ordinary_receipt_sum(work.live, work.recipe_heap);
	require(work.role.role == native_mobile_birth_cash_role::ordinary_wallet &&
		work.born.cash &&
		work.born.reference.birth_operation.bytes == command.operation_id.bytes);
	work.intent_wire = command.accounting_intent;
	checked(economic_intent_decode_bounded(work.intent_wire, &work.intent, reserve, context,
					       work.live));
	work.live = ordinary_receipt_sum(work.live, work.intent.admission.facts.capacity());
	checked(economic_intent_verify_binding_bounded(command, work.intent, reserve, context,
						       work.live));
	require(work.intent.admission.metadata.source_event.has_value());
	work.selected = current_source_claim_name_bounded(work.intent.admission.metadata, reserve,
							  context, work.live);
	work.live = ordinary_receipt_sum(work.live, ordinary_receipt_string_heap(work.selected));
	work.expected = current_source_claim_bytes_bounded(record, reserve, context, work.live);
	require(work.expected.size() == ordinary_claim_bytes);
	work.live = ordinary_receipt_sum(work.live, work.expected.capacity());
	work.path = ordinary_receipt_directory_bounded(root, reserve, context, work.live);
	work.live = ordinary_receipt_sum(work.live, ordinary_receipt_string_heap(work.path));
	receipt_bounded_admit(work.live, 0, reserve, context);
	ordinary_claim_file directory_fd(
		open(work.path.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW));
	require(directory_fd.value >= 0, errno == ELOOP ? status::invalid : status::io_error);
	struct stat directory_info
	{
	};
	require(fstat(directory_fd.value, &directory_info) == 0, status::io_error);
	require(S_ISDIR(directory_info.st_mode) && directory_info.st_uid == geteuid() &&
		!(directory_info.st_mode & 0077));
	ordinary_claim_directory entries(fdopendir(directory_fd.value));
	require(entries.value, status::io_error);
	directory_fd.value = -1;
	const int parent = dirfd(entries.value);
	require(parent >= 0, status::io_error);
	for (;;)
	{
		const size_t entry_frame =
			sizeof(std::string) + sizeof(struct dirent *) + 3 * sizeof(size_t) +
			sizeof(std::array<uint8_t, ordinary_claim_bytes>) + sizeof(reader) +
			sizeof(economic_operation_metadata) + 3 * sizeof(std::span<const uint8_t>) +
			sizeof(economic_source_event) +
			sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>) +
			sizeof(critical_operation_id) + sizeof(std::string);
		receipt_bounded_admit(work.live, entry_frame, reserve, context);
		errno = 0;
		const auto *entry = readdir(entries.value);
		if (!entry)
		{
			require(!errno, status::io_error);
			break;
		}
		const size_t length = strlen(entry->d_name);
		size_t entry_live = ordinary_receipt_sum(work.live, entry_frame);
		receipt_bounded_admit(entry_live, length > 15 ? ordinary_receipt_sum(length, 1) : 0,
				      reserve, context);
		const std::string name(entry->d_name);
		entry_live = ordinary_receipt_sum(entry_live, ordinary_receipt_string_heap(name));
		if (name.compare(0, sizeof("source-claim-") - 1, "source-claim-"))
			continue;
		receipt_bounded_admit(entry_live,
				      sizeof(size_t) * 2 + sizeof(void *) + sizeof(bool), reserve,
				      context);
		require(ordinary_claim_filename(name));
		// Fixed-array secure read has no C++ allocation. Admit every original
		// descriptor/stat/read-tail/return object before entering it.
		receipt_bounded_admit(entry_live,
				      sizeof(ordinary_claim_file) + 2 * sizeof(struct stat) +
					      sizeof(std::array<uint8_t, ordinary_claim_bytes>) +
					      sizeof(size_t) + 2 * sizeof(ssize_t) +
					      sizeof(uint8_t) + 2 * sizeof(int) + sizeof(void *),
				      reserve, context);
		const auto bytes = ordinary_claim_read(parent, name);
		receipt_bounded_admit(entry_live, receipt_unwrap_working, reserve, context);
		reader input{ unwrap(bytes, source_claim_magic, ordinary_claim_bytes) };
		economic_operation_metadata actual;
		const auto lineage = input.take(actual.lineage.bytes.size());
		std::copy(lineage.begin(), lineage.end(), actual.lineage.bytes.begin());
		require(!critical_operation_id_is_zero(actual.lineage));
		const auto encoded_event = input.take(ECONOMIC_SOURCE_EVENT_BYTES);
		economic_source_event decoded_event;
		checked(economic_source_event_decode(encoded_event, &decoded_event));
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> canonical_event{};
		checked(economic_source_event_encode(decoded_event, &canonical_event));
		require(std::equal(canonical_event.begin(), canonical_event.end(),
				   encoded_event.begin()));
		actual.source_event = decoded_event;
		critical_operation_id operation;
		const auto encoded_operation = input.take(operation.bytes.size());
		std::copy(encoded_operation.begin(), encoded_operation.end(),
			  operation.bytes.begin());
		require(!critical_operation_id_is_zero(operation));
		require(input.number(1) == 1 && input.number(7) == 0);
		input.done();
		const auto canonical_name =
			current_source_claim_name_bounded(actual, reserve, context, entry_live);
		require(name == canonical_name);
		if (operation.bytes == command.operation_id.bytes)
		{
			++work.operation_claims;
			require(work.operation_claims == 1, status::conflict);
		}
		if (name == work.selected)
		{
			require(!work.selected_found && std::equal(bytes.begin(), bytes.end(),
								   work.expected.begin()),
				status::conflict);
			work.selected_found = true;
		}
	}
	DIR *completed = entries.value;
	entries.value = nullptr;
	require(closedir(completed) == 0, status::io_error);
	require(lock.matches(root));
	require(work.operation_claims == 1 && work.selected_found,
		!work.selected_found ? status::not_found : status::conflict);
}
#endif
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI

// Bounded-only provider error law. Genuine callback/capacity and unsupported
// request-policy failures must not become damaged historical evidence.
// The original unbounded helper and all its original callers stay byte-exact.
void ordinary_receipt_authority_checked_bounded(unsigned int error)
{
	require(!error,
		error == ENOMEM || error == ENOSPC || error == EOVERFLOW || error == ENOBUFS ?
			status::capacity :
		error == EIO || error == ENOTSUP ? status::io_error :
						   status::invalid);
}
struct ordinary_receipt_retained_workspace
{
	context current;
	flatfile_accounting_record record;
	quest_mobile_native_image born;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_frozen_intent intent;
	std::span<const uint8_t> intent_wire;
	native_mobile_birth_cash_role_result receipt, expected_receipt;
	economic_account_key wallet;
	economic_accounting_plan expected;
	std::vector<uint8_t> plan_bytes;
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> receipt_bytes{};
	std::vector<economic_accounting_item_reference> references;
	flatfile_native_mobile_birth_ordinary_reference_current reference_counts;
	size_t image_heap = 0, recipe_heap = 0, plan_heap = 0, record_heap = 0, live = 0;
	unsigned int authority_error = 0;
	flatfile_item_accounting_status references_error{};
	status claim_error{};
};
struct ordinary_receipt_history_workspace
{
	flatfile_accounting_record record;
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	native_mobile_birth_cash_role_result receipt;
	economic_frozen_intent intent;
	std::span<const uint8_t> intent_wire;
	economic_account_key wallet;
	economic_accounting_plan expected;
	std::vector<uint8_t> expected_plan, original_record;
	std::vector<uint64_t> born;
	flatfile_native_mobile_birth_ordinary_retained_metadata metadata;
	flatfile_ordinary_native_birth_economic_history_counts observed;
	size_t image_heap = 0, recipe_heap = 0, plan_heap = 0, record_heap = 0, metadata_heap = 0,
	       live = 0;
	status receipt_status{}, claim_status{};
};
struct ordinary_receipt_history_visit
{
	const critical_operation_id &operation;
	ordinary_receipt_history_workspace &work;
	flatfile_scratch_reserve_fn reserve;
	void *context;
	size_t outer;
	void operator()(const flatfile_accounting_record &retained,
			const economic_accounting_plan *plan, std::span<const uint8_t> wire) const
	{
		const size_t frame =
			4 * sizeof(void *) + sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t);
		receipt_bounded_admit(outer, frame, reserve, context);
		if (retained.command.operation_id.bytes == operation.bytes)
		{
			require(plan && work.original_record.size() == wire.size() &&
				std::equal(work.original_record.begin(), work.original_record.end(),
					   wire.begin()));
			ordinary_receipt_creation_bounded(retained.command, work.image, *plan,
							  reserve, context,
							  ordinary_receipt_sum(outer, frame));
			require(retained.command.source_site == work.record.command.source_site &&
				retained.plan == work.expected_plan &&
				plan->accounts.size() == work.expected.accounts.size() &&
				plan->postings.size() == work.expected.postings.size() &&
				plan->children.empty());
			++work.observed.birth_records_verified;
			work.observed.creation_events_verified += plan->item_events.size();
			work.observed.birth_account_effects_verified += plan->accounts.size();
			work.observed.birth_coin_postings_verified += plan->postings.size();
			return;
		}
		if (!plan)
			return;
		for (const auto &event : plan->item_events)
			require(event.after.revision != 1 ||
					!std::binary_search(work.born.begin(), work.born.end(),
							    event.uid),
				status::conflict);
		if (retained.command.type == critical_command_type::economic_baseline)
			work.observed.baseline_witness_rows_verified +=
				plan->items_before.size() + plan->items_after.size();
	}
};
struct ordinary_receipt_scan_workspace
{
	std::set<std::array<uint8_t, 16>> retained_epochs;
	size_t live = 0, bucket = 0;
	bool initialized = false;
};
struct ordinary_receipt_scan_bucket_workspace
{
	context value;
	size_t live = 0;
	uint32_t active = 0, segment = 0;
};
struct ordinary_receipt_scan_segment_workspace
{
	std::vector<uint8_t> bytes;
	std::vector<const entry *> entries;
	size_t live = 0;
};
struct ordinary_receipt_scan_record_workspace
{
	std::span<const uint8_t> wire, intent_wire, plan_wire;
	flatfile_accounting_record record;
	economic_frozen_intent intent;
	std::vector<uint8_t> canonical;
	economic_accounting_plan plan;
	size_t live = 0, plan_heap = 0;
};
void ordinary_receipt_history_scan_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_native_mobile_birth_ordinary_retained_metadata &metadata,
	flatfile_ordinary_native_birth_economic_history_counts &observed,
	ordinary_receipt_history_visit &visit, flatfile_scratch_reserve_fn reserve,
	void *context_pointer, size_t outer)
{
	require(!root.empty() && lock.matches(root));
	const size_t frame = sizeof(ordinary_receipt_scan_workspace) + 6 * sizeof(void *) +
			     sizeof(flatfile_scratch_reserve_fn) + 2 * sizeof(size_t);
	receipt_bounded_admit(outer, frame, reserve, context_pointer);
	ordinary_receipt_scan_workspace scan;
	scan.live = ordinary_receipt_sum(outer, frame);
	for (const auto &epoch : metadata.epochs)
	{
		// Each original set insertion owns one real rb-tree node, with all
		// previously inserted retained epoch nodes still live.
		require(scan.retained_epochs.size() <
				SIZE_MAX / sizeof(std::_Rb_tree_node<std::array<uint8_t, 16>>),
			status::capacity);
		receipt_bounded_admit(
			scan.live,
			(scan.retained_epochs.size() + 1) *
					sizeof(std::_Rb_tree_node<std::array<uint8_t, 16>>) +
				sizeof(void *) +
				sizeof(std::pair<decltype(scan.retained_epochs.begin()), bool>),
			reserve, context_pointer);
		require(scan.retained_epochs.insert(epoch.epoch.bytes).second);
	}
	scan.live = ordinary_receipt_sum(
		scan.live,
		scan.retained_epochs.size() * sizeof(std::_Rb_tree_node<std::array<uint8_t, 16>>));
	require(metadata.epochs.size() == metadata.control.epoch_count);
	observed.lineage = metadata.control.lineage;
	observed.lineage_revision = metadata.control.revision;
	observed.epochs_digest = metadata.control.epochs_digest;
	observed.retained_epochs = static_cast<uint32_t>(metadata.epochs.size());
	for (scan.bucket = 0; scan.bucket < FLATFILE_ACCOUNTING_BUCKETS; ++scan.bucket)
	{
		scan.initialized = (metadata.control.evidence_initialized[scan.bucket / 8] &
				    (uint8_t{ 1 } << (scan.bucket % 8))) != 0;
		if (!scan.initialized)
		{
			ordinary_receipt_empty_bucket_bounded(root, scan.bucket, reserve,
							      context_pointer, scan.live);
			++observed.clear_buckets;
		}
		else
		{
			receipt_bounded_admit(scan.live,
					      sizeof(ordinary_receipt_scan_bucket_workspace),
					      reserve, context_pointer);
			ordinary_receipt_scan_bucket_workspace bucket;
			bucket.live = ordinary_receipt_sum(scan.live, sizeof(bucket));
			bucket.value = receipt_load_context_bounded(root, scan.bucket, reserve,
								    context_pointer, bucket.live);
			bucket.live = ordinary_receipt_sum(
				bucket.live,
				ordinary_receipt_vector_heap(bucket.value.index.entries));
			bucket.live =
				ordinary_receipt_sum(bucket.live, bucket.value.active.capacity());
			require(bucket.value.index.lineage.bytes == metadata.control.lineage.bytes,
				status::conflict);
			++observed.initialized_buckets;
			if (!bucket.value.index.entries.empty())
			{
				bucket.active = last_segment(bucket.value.index);
				for (bucket.segment = 0; bucket.segment <= bucket.active;
				     ++bucket.segment)
				{
					receipt_bounded_admit(
						bucket.live,
						sizeof(ordinary_receipt_scan_segment_workspace),
						reserve, context_pointer);
					ordinary_receipt_scan_segment_workspace segment;
					segment.live =
						ordinary_receipt_sum(bucket.live, sizeof(segment));
					if (bucket.segment == bucket.active)
					{
						segment.bytes = std::move(bucket.value.active);
						// The active payload was already charged in bucket.live.
					}
					else
					{
						segment.bytes = receipt_load_segment_bounded(
							root, bucket.value.index, bucket.segment,
							reserve, context_pointer, segment.live);
						segment.live = ordinary_receipt_sum(
							segment.live, segment.bytes.capacity());
					}
					receipt_bounded_admit(
						segment.live,
						receipt_segment_entries_working(bucket.value.index,
										bucket.segment),
						reserve, context_pointer);
					segment.entries =
						segment_entries(bucket.value.index, bucket.segment);
					segment.live = ordinary_receipt_sum(
						segment.live,
						ordinary_receipt_vector_heap(segment.entries));
					for (const auto *item : segment.entries)
					{
						receipt_bounded_admit(
							segment.live,
							sizeof(ordinary_receipt_scan_record_workspace) +
								sizeof(const entry *),
							reserve, context_pointer);
						ordinary_receipt_scan_record_workspace row;
						row.live = ordinary_receipt_sum(
							segment.live, sizeof(row) + sizeof(item));
						row.wire = std::span<const uint8_t>(segment.bytes)
								   .subspan(header_bytes + 32 +
										    item->offset,
									    item->bytes);
						row.record = receipt_decode_record_bounded(
							row.wire, reserve, context_pointer,
							row.live);
						row.live = ordinary_receipt_sum(
							row.live, receipt_record_heap(row.record));
						row.intent_wire =
							row.record.command.accounting_intent;
						checked(economic_intent_decode_bounded(
							row.intent_wire, &row.intent, reserve,
							context_pointer, row.live));
						row.live = ordinary_receipt_sum(
							row.live,
							row.intent.admission.facts.capacity());
						checked(economic_intent_verify_binding_bounded(
							row.record.command, row.intent, reserve,
							context_pointer, row.live));
						require(row.record.command.operation_id.bytes ==
								item->id.bytes &&
							row.intent.admission.metadata.lineage.bytes ==
								bucket.value.index.lineage.bytes &&
							scan.retained_epochs.contains(
								row.intent.admission.metadata.epoch
									.bytes));
						row.canonical =
							ordinary_receipt_record_encode_bounded(
								row.record, reserve,
								context_pointer, row.live);
						row.live = ordinary_receipt_sum(
							row.live, row.canonical.capacity());
						require(row.canonical.size() == row.wire.size() &&
							std::equal(row.canonical.begin(),
								   row.canonical.end(),
								   row.wire.begin()));
						++observed.records_verified;
						if (row.record.result_code)
						{
							++observed.failed_records;
							visit.outer = row.live;
							visit(row.record, nullptr, row.wire);
						}
						else
						{
							row.plan_wire = row.record.plan;
							checked(economic_plan_decode_bounded(
								row.plan_wire, &row.plan, reserve,
								context_pointer, row.live,
								&row.plan_heap));
							row.live = ordinary_receipt_sum(
								row.live, row.plan_heap);
							require(row.plan.metadata.operation_id
										.bytes ==
									row.record.command
										.operation_id
										.bytes &&
								row.plan.metadata.lineage.bytes ==
									metadata.control.lineage
										.bytes &&
								row.plan.metadata.epoch.bytes ==
									row.intent.admission
										.metadata.epoch
										.bytes &&
								row.plan.children.empty());
							++observed.successful_records;
							observed.item_events_verified +=
								row.plan.item_events.size();
							visit.outer = row.live;
							visit(row.record, &row.plan, row.wire);
						}
					}
					++observed.segments_verified;
				}
			}
		}
		++observed.buckets_verified;
	}
	require(observed.buckets_verified == FLATFILE_ACCOUNTING_BUCKETS && lock.matches(root));
}
#endif
}

flatfile_accounting_status
flatfile_ordinary_native_birth_receipt_storage::verify_retained_current_locked_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation, flatfile_accounting_record *output,
	flatfile_scratch_reserve_fn reserve, void *context_pointer, size_t outer,
	size_t *retained_output_payload_bytes) noexcept
{
	if (!output || !reserve)
		return status::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)operation;
	(void)context_pointer;
	(void)outer;
	(void)retained_output_payload_bytes;
	errno = ENOTSUP;
	return status::io_error;
#else
	ordinary_receipt_forward forward{ reserve, context_pointer };
	try
	{
		require(!root.empty() && lock.matches(root) &&
			!critical_operation_id_is_zero(operation));
		const size_t frame = sizeof(ordinary_receipt_retained_workspace) +
				     sizeof(ordinary_receipt_forward) +
				     ordinary_receipt_secure_read_source_frames +
				     ordinary_receipt_result_source_frames + 6 * sizeof(void *) +
				     sizeof(flatfile_scratch_reserve_fn) + 2 * sizeof(size_t) +
				     sizeof(std::span<const economic_accounting_item_reference>);
		receipt_bounded_admit(outer, frame, reserve, context_pointer);
		reserve = ordinary_receipt_forward::admit;
		context_pointer = &forward;
		ordinary_receipt_retained_workspace work;
		work.live = ordinary_receipt_sum(outer, frame);
		work.current = receipt_load_context_bounded(root, bucket_for(operation), reserve,
							    context_pointer, work.live);
		work.live = ordinary_receipt_sum(
			work.live, ordinary_receipt_vector_heap(work.current.index.entries));
		work.live = ordinary_receipt_sum(work.live, work.current.active.capacity());
		work.record = receipt_retained_in_bounded(root, work.current, operation, reserve,
							  context_pointer, work.live);
		work.record_heap = receipt_record_heap(work.record);
		work.live = ordinary_receipt_sum(work.live, work.record_heap);
		require(work.record.command.schema_version ==
				CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			work.record.command.type == critical_command_type::native_mobile_birth &&
			work.record.command.payload_version ==
				NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
			!work.record.result_code &&
			work.record.failure_stage == critical_failure_stage::none &&
			work.record.durable_revision == 1 &&
			work.record.result.size() == NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES);
		checked(native_mobile_birth_cash_role_command_decode_bounded(
			work.record.command, &work.born, &work.recipes, &work.role, reserve,
			context_pointer, work.live, &work.image_heap, &work.recipe_heap));
		work.live = ordinary_receipt_sum(work.live, work.image_heap);
		work.live = ordinary_receipt_sum(work.live, work.recipe_heap);
		require(work.role.role == native_mobile_birth_cash_role::ordinary_wallet &&
			work.born.cash &&
			work.born.reference.birth_operation.bytes == operation.bytes);
		work.intent_wire = work.record.command.accounting_intent;
		checked(economic_intent_decode_bounded(work.intent_wire, &work.intent, reserve,
						       context_pointer, work.live));
		work.live = ordinary_receipt_sum(work.live, work.intent.admission.facts.capacity());
		checked(economic_intent_verify_binding_bounded(
			work.record.command, work.intent, reserve, context_pointer, work.live));
		require(work.intent.admission.metadata.source_event.has_value());
		require(native_mobile_birth_cash_role_result_decode(work.record.result,
								    &work.receipt) &&
			work.receipt.role == native_mobile_birth_cash_role::ordinary_wallet);
		work.wallet = { work.intent.admission.metadata.lineage,
				economic_account_kind::wallet, work.receipt.wallet_mapping_id,
				ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		checked(native_mobile_birth_cash_role_accounting_compile_bounded(
			work.record.command, work.wallet, &work.expected, reserve, context_pointer,
			work.live, &work.plan_heap));
		work.live = ordinary_receipt_sum(work.live, work.plan_heap);
		ordinary_receipt_plan_encode_bounded(work.expected, &work.plan_bytes, reserve,
						     context_pointer, work.live);
		work.live = ordinary_receipt_sum(work.live, work.plan_bytes.capacity());
		require(work.plan_bytes == work.record.plan && work.expected.children.empty());
		checked(native_mobile_birth_cash_role_result_build_bounded(
			work.record.command, work.wallet, work.expected, &work.expected_receipt,
			reserve, context_pointer, work.live));
		require(native_mobile_birth_cash_role_result_encode(work.expected_receipt,
								    &work.receipt_bytes) &&
			std::equal(work.receipt_bytes.begin(), work.receipt_bytes.end(),
				   work.record.result.begin()));
		work.authority_error =
			flatfile_ordinary_native_birth_history_storage::verify_locked_bounded(
				root, lock, work.wallet, work.intent.admission.metadata.epoch,
				operation, work.born.reference.mobile_instance_id, reserve,
				context_pointer, work.live);
		ordinary_receipt_authority_checked_bounded(work.authority_error);
		require(work.expected.item_events.size() <=
				SIZE_MAX / sizeof(economic_accounting_item_reference),
			status::capacity);
		receipt_bounded_admit(work.live,
				      work.expected.item_events.size() *
						      sizeof(economic_accounting_item_reference) +
					      sizeof(economic_accounting_item_reference) +
					      sizeof(void *),
				      reserve, context_pointer);
		work.references.reserve(work.expected.item_events.size());
		work.live = ordinary_receipt_sum(work.live,
						 ordinary_receipt_vector_heap(work.references));
		for (const auto &event : work.expected.item_events)
		{
			require(event.event_index < UINT16_MAX);
			economic_accounting_item_reference reference;
			reference.operation_id = operation;
			reference.line_index = static_cast<uint16_t>(event.event_index);
			reference.event_index = event.event_index;
			reference.child_index = event.child_index;
			reference.item_uid = event.uid;
			reference.before_revision = event.before.revision;
			reference.after_revision = event.after.revision;
			reference.legacy_operation_id = operation;
			reference.legacy_event_index = reference.line_index;
			require(economic_accounting_item_reference_validate(reference));
			work.references.push_back(reference);
		}
		work.references_error =
			flatfile_native_mobile_birth_ordinary_reference_history_storage::
				verify_current_operation_locked_bounded(root, lock, operation,
									work.references,
									&work.reference_counts,
									reserve, context_pointer,
									work.live);
		require(work.references_error == flatfile_item_accounting_status::ok,
			work.references_error == flatfile_item_accounting_status::capacity ?
				status::capacity :
			work.references_error == flatfile_item_accounting_status::io_error ?
				status::io_error :
			work.references_error == flatfile_item_accounting_status::not_found ?
				status::not_found :
				status::invalid);
		work.claim_error = flatfile_accounting_storage::verify_source_claim_bounded(
			root, lock, work.record, reserve, context_pointer, work.live);
		require(work.claim_error == status::ok, work.claim_error);
		require(lock.matches(root));
		receipt_bounded_admit(work.live, 0, reserve, context_pointer);
		static_assert(std::is_nothrow_move_assignable_v<flatfile_accounting_record>);
		*output = std::move(work.record);
		if (retained_output_payload_bytes)
			*retained_output_payload_bytes = work.record_heap;
		return status::ok;
	}
	catch (const failure &value)
	{
		return forward.refused ? status::capacity : value.code;
	}
	catch (const std::bad_alloc &)
	{
		return status::capacity;
	}
	catch (...)
	{
		return status::io_error;
	}
#endif
}

flatfile_accounting_status
flatfile_ordinary_native_birth_receipt_storage::verify_retained_history_current_locked_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation, flatfile_accounting_record *output,
	flatfile_ordinary_native_birth_economic_history_counts *counts,
	flatfile_scratch_reserve_fn reserve, void *context_pointer, size_t outer,
	size_t *retained_output_payload_bytes) noexcept
{
	if (!output || !counts || !reserve)
		return status::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)operation;
	(void)context_pointer;
	(void)outer;
	(void)retained_output_payload_bytes;
	errno = ENOTSUP;
	return status::io_error;
#else
	ordinary_receipt_forward forward{ reserve, context_pointer };
	try
	{
		require(!root.empty() && lock.matches(root) &&
			!critical_operation_id_is_zero(operation));
		const size_t frame = sizeof(ordinary_receipt_history_workspace) +
				     sizeof(ordinary_receipt_forward) +
				     ordinary_receipt_secure_read_source_frames +
				     ordinary_receipt_result_source_frames +
				     sizeof(ordinary_receipt_history_visit) + 7 * sizeof(void *) +
				     sizeof(flatfile_scratch_reserve_fn) + 2 * sizeof(size_t);
		receipt_bounded_admit(outer, frame, reserve, context_pointer);
		reserve = ordinary_receipt_forward::admit;
		context_pointer = &forward;
		ordinary_receipt_history_workspace work;
		work.live = ordinary_receipt_sum(outer, frame);
		work.receipt_status = verify_retained_current_locked_bounded(
			root, lock, operation, &work.record, reserve, context_pointer, work.live,
			&work.record_heap);
		require(work.receipt_status == status::ok, work.receipt_status);
		work.live = ordinary_receipt_sum(work.live, work.record_heap);
		work.claim_status = verify_source_claim_current_locked_bounded(
			root, lock, work.record, reserve, context_pointer, work.live);
		require(work.claim_status == status::ok, work.claim_status);
		checked(native_mobile_birth_cash_role_command_decode_bounded(
			work.record.command, &work.image, &work.recipes, &work.role, reserve,
			context_pointer, work.live, &work.image_heap, &work.recipe_heap));
		work.live = ordinary_receipt_sum(work.live, work.image_heap);
		work.live = ordinary_receipt_sum(work.live, work.recipe_heap);
		require(work.role.role == native_mobile_birth_cash_role::ordinary_wallet);
		require(native_mobile_birth_cash_role_result_decode(work.record.result,
								    &work.receipt) &&
			work.receipt.role == native_mobile_birth_cash_role::ordinary_wallet);
		work.intent_wire = work.record.command.accounting_intent;
		checked(economic_intent_decode_bounded(work.intent_wire, &work.intent, reserve,
						       context_pointer, work.live));
		work.live = ordinary_receipt_sum(work.live, work.intent.admission.facts.capacity());
		checked(economic_intent_verify_binding_bounded(
			work.record.command, work.intent, reserve, context_pointer, work.live));
		work.wallet = { work.intent.admission.metadata.lineage,
				economic_account_kind::wallet, work.receipt.wallet_mapping_id,
				ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		checked(native_mobile_birth_cash_role_accounting_compile_bounded(
			work.record.command, work.wallet, &work.expected, reserve, context_pointer,
			work.live, &work.plan_heap));
		work.live = ordinary_receipt_sum(work.live, work.plan_heap);
		ordinary_receipt_plan_encode_bounded(work.expected, &work.expected_plan, reserve,
						     context_pointer, work.live);
		work.live = ordinary_receipt_sum(work.live, work.expected_plan.capacity());
		require(work.expected_plan == work.record.plan);
		ordinary_receipt_creation_bounded(work.record.command, work.image, work.expected,
						  reserve, context_pointer, work.live);
		work.original_record = ordinary_receipt_record_encode_bounded(
			work.record, reserve, context_pointer, work.live);
		work.live = ordinary_receipt_sum(work.live, work.original_record.capacity());
		work.born = ordinary_receipt_born_bounded(work.image, reserve, context_pointer,
							  work.live);
		work.live =
			ordinary_receipt_sum(work.live, ordinary_receipt_vector_heap(work.born));
		ordinary_receipt_authority_checked_bounded(
			flatfile_native_mobile_birth_ordinary_baseline_history_storage::
				read_metadata_locked_bounded(root, lock, &work.metadata, reserve,
							     context_pointer, work.live,
							     &work.metadata_heap));
		work.live = ordinary_receipt_sum(work.live, work.metadata_heap);
		require(work.metadata.control.lineage.bytes ==
			work.expected.metadata.lineage.bytes);
		work.observed.born_uids = work.born.size();
		ordinary_receipt_history_visit visit{ operation, work, reserve, context_pointer,
						      work.live };
		ordinary_receipt_history_scan_bounded(root, lock, work.metadata, work.observed,
						      visit, reserve, context_pointer, work.live);
		require(work.observed.birth_records_verified == 1 &&
			work.observed.creation_events_verified ==
				work.expected.item_events.size() &&
			work.observed.birth_account_effects_verified ==
				work.expected.accounts.size() &&
			work.observed.birth_coin_postings_verified ==
				work.expected.postings.size() &&
			lock.matches(root));
		receipt_bounded_admit(work.live, 0, reserve, context_pointer);
		static_assert(std::is_nothrow_move_assignable_v<flatfile_accounting_record>);
		static_assert(std::is_nothrow_copy_assignable_v<
			      flatfile_ordinary_native_birth_economic_history_counts>);
		*output = std::move(work.record);
		*counts = work.observed;
		if (retained_output_payload_bytes)
			*retained_output_payload_bytes = work.record_heap;
		return status::ok;
	}
	catch (const failure &value)
	{
		return forward.refused ? status::capacity : value.code;
	}
	catch (const std::bad_alloc &)
	{
		return status::capacity;
	}
	catch (...)
	{
		return status::io_error;
	}
#endif
}

flatfile_accounting_status
flatfile_ordinary_native_birth_receipt_storage::verify_source_claim_current_locked_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_record &record, flatfile_scratch_reserve_fn reserve,
	void *context, size_t outer) noexcept
{
	if (!reserve)
		return status::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)record;
	(void)context;
	(void)outer;
	errno = ENOTSUP;
	return status::io_error;
#else
	ordinary_receipt_forward forward{ reserve, context };
	try
	{
		require(!record.result_code &&
			record.failure_stage == critical_failure_stage::none &&
			record.durable_revision == 1 &&
			record.result.size() == NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES);
		const size_t frame = sizeof(ordinary_receipt_forward) +
				     ordinary_receipt_secure_read_source_frames +
				     ordinary_receipt_result_source_frames + 4 * sizeof(void *) +
				     sizeof(flatfile_scratch_reserve_fn) + 2 * sizeof(size_t);
		receipt_bounded_admit(outer, frame, reserve, context);
		reserve = ordinary_receipt_forward::admit;
		context = &forward;
		ordinary_receipt_claim_census_bounded(root, lock, record, reserve, context,
						      ordinary_receipt_sum(outer, frame));
		return status::ok;
	}
	catch (const failure &value)
	{
		return forward.refused ? status::capacity : value.code;
	}
	catch (const std::bad_alloc &)
	{
		return status::capacity;
	}
	catch (...)
	{
		return status::io_error;
	}
#endif
}
