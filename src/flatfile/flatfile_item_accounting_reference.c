#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_reference_history.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include <array>
#include <set>
#include <utility>
#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <system_error>

namespace
{
bool ensure_directory(const std::string &dir)
{
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);
	if (ec)
		return false;
	const auto parent = std::filesystem::path(dir).parent_path().string();
	return chmod(parent.c_str(), 0700) == 0 && chmod(dir.c_str(), 0700) == 0;
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

	bool can_take(size_t size) const
	{
		return offset <= bytes.size() && size <= bytes.size() - offset;
	}

	std::span<const uint8_t> take(size_t size)
	{
		if (!can_take(size))
			return {};
		auto result = bytes.subspan(offset, size);
		offset += size;
		return result;
	}

	uint64_t read_number(size_t width)
	{
		auto chunk = take(width);
		if (chunk.size() < width)
			return 0;
		uint64_t value = 0;
		for (size_t i = 0; i < width; ++i)
			value |= static_cast<uint64_t>(chunk[i]) << (8 * i);
		return value;
	}
};

std::string bucket_filename(uint8_t bucket)
{
	char buf[16] = {};
	std::snprintf(buf, sizeof(buf), "%02x.bin", bucket);
	return std::string(buf);
}

std::string item_refs_directory(const std::string &root)
{
	return root + "/accounting/item_references";
}

flatfile_item_accounting_status read_status(flatfile_read_result result)
{
	switch (result)
	{
	case flatfile_read_result::ok:
		return flatfile_item_accounting_status::ok;
	case flatfile_read_result::not_found:
		return flatfile_item_accounting_status::not_found;
	case flatfile_read_result::invalid:
		return flatfile_item_accounting_status::invalid;
	case flatfile_read_result::io_error:
		return flatfile_item_accounting_status::io_error;
	}
	return flatfile_item_accounting_status::io_error;
}

bool valid_bucket(std::span<const uint8_t> bytes, uint8_t bucket,
		  const economic_accounting_item_reference *legacy_key = nullptr,
		  size_t *legacy_matches = nullptr)
{
	if (legacy_matches)
		*legacy_matches = 0;
	if (bytes.size() % FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
		return false;
	for (size_t offset = 0; offset < bytes.size();
	     offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
	{
		economic_accounting_item_reference reference = {};
		const auto record =
			bytes.subspan(offset, FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
		if (flatfile_item_accounting_reference_decode(record, &reference) !=
			    flatfile_item_accounting_status::ok ||
		    reference.legacy_operation_id.bytes[0] != bucket)
			return false;
		if (legacy_key && legacy_matches &&
		    reference.legacy_operation_id.bytes == legacy_key->legacy_operation_id.bytes &&
		    reference.legacy_event_index == legacy_key->legacy_event_index)
			++*legacy_matches;
	}
	return true;
}
} // namespace

flatfile_item_accounting_status
flatfile_item_accounting_reference_encode(const economic_accounting_item_reference &ref,
					  std::vector<uint8_t> *bytes)
{
	if (!bytes)
		return flatfile_item_accounting_status::invalid;
	if (!economic_accounting_item_reference_validate(ref))
		return flatfile_item_accounting_status::invalid;

	bytes->clear();
	bytes->reserve(FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);

	raw(*bytes, ref.operation_id.bytes);
	number(*bytes, ref.line_index, 2);
	number(*bytes, ref.event_index, 4);
	number(*bytes, ref.child_index, 2);
	number(*bytes, ref.item_uid, 8);
	number(*bytes, ref.before_revision, 8);
	number(*bytes, ref.after_revision, 8);
	raw(*bytes, ref.legacy_operation_id.bytes);
	number(*bytes, ref.legacy_event_index, 2);

	if (bytes->size() != FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
		return flatfile_item_accounting_status::capacity;

	return flatfile_item_accounting_status::ok;
}

flatfile_item_accounting_status
flatfile_item_accounting_reference_decode(std::span<const uint8_t> bytes,
					  economic_accounting_item_reference *ref)
{
	if (!ref || bytes.size() != FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
		return flatfile_item_accounting_status::invalid;

	reader r{ bytes, 0 };
	auto op_id = r.take(16);
	if (op_id.size() != 16)
		return flatfile_item_accounting_status::invalid;

	uint16_t line_index = static_cast<uint16_t>(r.read_number(2));
	uint32_t event_index = static_cast<uint32_t>(r.read_number(4));
	uint16_t child_index = static_cast<uint16_t>(r.read_number(2));
	uint64_t item_uid = r.read_number(8);
	uint64_t before_revision = r.read_number(8);
	uint64_t after_revision = r.read_number(8);
	auto legacy_op_id = r.take(16);
	if (legacy_op_id.size() != 16)
		return flatfile_item_accounting_status::invalid;
	uint16_t legacy_event_index = static_cast<uint16_t>(r.read_number(2));

	economic_accounting_item_reference decoded = {};
	std::copy(op_id.begin(), op_id.end(), decoded.operation_id.bytes.begin());
	decoded.line_index = line_index;
	decoded.event_index = event_index;
	decoded.child_index = child_index;
	decoded.item_uid = item_uid;
	decoded.before_revision = before_revision;
	decoded.after_revision = after_revision;
	std::copy(legacy_op_id.begin(), legacy_op_id.end(),
		  decoded.legacy_operation_id.bytes.begin());
	decoded.legacy_event_index = legacy_event_index;

	if (!economic_accounting_item_reference_validate(decoded))
		return flatfile_item_accounting_status::invalid;

	*ref = decoded;
	return flatfile_item_accounting_status::ok;
}

flatfile_item_accounting_status flatfile_item_accounting_reference_append(
	const std::string &root, const economic_accounting_item_reference &ref, std::string *error)
{
	std::vector<uint8_t> encoded;
	auto status = flatfile_item_accounting_reference_encode(ref, &encoded);
	if (status != flatfile_item_accounting_status::ok)
	{
		if (error)
			*error = "failed to encode item accounting reference";
		return status;
	}

	const std::string dir = item_refs_directory(root);
	if (!ensure_directory(dir))
		return flatfile_item_accounting_status::io_error;
	const std::string filename = bucket_filename(ref.legacy_operation_id.bytes[0]);

	std::vector<uint8_t> existing;
	auto read_result = flatfile_read(dir, filename,
					 FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
					 &existing, error);
	if (read_result != flatfile_read_result::ok &&
	    read_result != flatfile_read_result::not_found)
		return read_status(read_result);
	if (existing.size() % FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
	{
		if (error)
			*error = "invalid partial item accounting reference record";
		return flatfile_item_accounting_status::invalid;
	}
	if (encoded.size() > FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES - existing.size())
		return flatfile_item_accounting_status::capacity;
	size_t duplicate_count = 0;
	if (!valid_bucket(existing, ref.legacy_operation_id.bytes[0], &ref, &duplicate_count))
	{
		if (error)
			*error = "invalid item accounting reference bucket";
		return flatfile_item_accounting_status::invalid;
	}
	if (duplicate_count)
		return flatfile_item_accounting_status::already_exists;

	existing.insert(existing.end(), encoded.begin(), encoded.end());

	if (!flatfile_atomic_write(dir, filename, existing, error))
		return flatfile_item_accounting_status::io_error;

	return flatfile_item_accounting_status::ok;
}

flatfile_item_accounting_status flatfile_item_accounting_reference_stage(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &legacy_operation_id,
	std::span<const economic_accounting_item_reference> references,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !operations ||
	    critical_operation_id_is_zero(legacy_operation_id))
		return flatfile_item_accounting_status::invalid;
	const critical_operation_id &operation_id = legacy_operation_id;
	const uint8_t bucket = operation_id.bytes[0];

	std::vector<uint8_t> addition;
	addition.reserve(references.size() * FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
	std::vector<uint16_t> event_indexes;
	event_indexes.reserve(references.size());
	for (const auto &reference : references)
	{
		if (reference.legacy_operation_id.bytes != operation_id.bytes)
			return flatfile_item_accounting_status::invalid;
		std::vector<uint8_t> encoded;
		const auto status = flatfile_item_accounting_reference_encode(reference, &encoded);
		if (status != flatfile_item_accounting_status::ok)
			return status;
		if (std::find(event_indexes.begin(), event_indexes.end(),
			      reference.legacy_event_index) != event_indexes.end())
			return flatfile_item_accounting_status::invalid;
		event_indexes.push_back(reference.legacy_event_index);
		addition.insert(addition.end(), encoded.begin(), encoded.end());
	}

	const std::string dir = item_refs_directory(root);
	if (!ensure_directory(dir))
		return flatfile_item_accounting_status::io_error;
	const std::string filename = bucket_filename(bucket);
	std::vector<uint8_t> existing;
	size_t staged_index = operations->size();
	for (size_t index = 0; index < operations->size(); ++index)
		if ((*operations)[index].store ==
			    flatfile_authority_store::item_accounting_references &&
		    (*operations)[index].filename == filename)
		{
			if (staged_index != operations->size() ||
			    (*operations)[index].kind != flatfile_authority_operation_kind::write)
				return flatfile_item_accounting_status::invalid;
			staged_index = index;
		}
	if (staged_index < operations->size())
		existing = (*operations)[staged_index].bytes;
	else
	{
		if (!references.empty() &&
		    operations->size() >= flatfile_authority_transaction_maximum_operations)
			return flatfile_item_accounting_status::capacity;
		const auto read_result = flatfile_read(
			dir, filename, FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
			&existing, error);
		if (read_result != flatfile_read_result::ok &&
		    read_result != flatfile_read_result::not_found)
			return read_status(read_result);
	}
	if (!valid_bucket(existing, bucket))
		return flatfile_item_accounting_status::invalid;
	for (size_t offset = 0; offset < existing.size();
	     offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
	{
		economic_accounting_item_reference retained = {};
		if (flatfile_item_accounting_reference_decode(
			    std::span<const uint8_t>(existing).subspan(
				    offset, FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES),
			    &retained) != flatfile_item_accounting_status::ok)
			return flatfile_item_accounting_status::invalid;
		if (retained.legacy_operation_id.bytes == operation_id.bytes)
			return flatfile_item_accounting_status::already_exists;
	}
	if (addition.size() > FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES - existing.size())
		return flatfile_item_accounting_status::capacity;
	if (references.empty())
		return flatfile_item_accounting_status::ok;
	if (addition.size() > flatfile_authority_transaction_maximum_bytes - existing.size())
		return flatfile_item_accounting_status::capacity;
	existing.insert(existing.end(), addition.begin(), addition.end());
	auto candidate = *operations;
	if (staged_index < candidate.size())
		candidate[staged_index].bytes = std::move(existing);
	else
		candidate.push_back({ flatfile_authority_store::item_accounting_references,
				      flatfile_authority_operation_kind::write, filename,
				      std::move(existing) });
	*operations = std::move(candidate);
	return flatfile_item_accounting_status::ok;
}
catch (const std::bad_alloc &)
{
	return flatfile_item_accounting_status::capacity;
}

flatfile_item_accounting_status flatfile_item_accounting_reference_verify_operation(
	const std::string &root, const critical_operation_id &legacy_operation_id,
	std::span<const economic_accounting_item_reference> expected, std::string *error)
try
{
	if (root.empty() || critical_operation_id_is_zero(legacy_operation_id))
		return flatfile_item_accounting_status::invalid;
	const uint8_t bucket = legacy_operation_id.bytes[0];
	std::vector<uint8_t> existing;
	const auto read_result = flatfile_read(item_refs_directory(root), bucket_filename(bucket),
					       FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
					       &existing, error);
	if (read_result == flatfile_read_result::not_found)
		return expected.empty() ? flatfile_item_accounting_status::ok :
					  flatfile_item_accounting_status::not_found;
	if (read_result != flatfile_read_result::ok)
		return read_status(read_result);
	if (!valid_bucket(existing, bucket))
		return flatfile_item_accounting_status::invalid;
	std::vector<economic_accounting_item_reference> retained;
	for (size_t offset = 0; offset < existing.size();
	     offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
	{
		economic_accounting_item_reference reference = {};
		if (flatfile_item_accounting_reference_decode(
			    std::span<const uint8_t>(existing).subspan(
				    offset, FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES),
			    &reference) != flatfile_item_accounting_status::ok)
			return flatfile_item_accounting_status::invalid;
		if (reference.legacy_operation_id.bytes == legacy_operation_id.bytes)
			retained.push_back(reference);
	}
	if (retained.size() != expected.size())
		return flatfile_item_accounting_status::invalid;
	auto less = [](const auto &left, const auto &right)
	{ return left.legacy_event_index < right.legacy_event_index; };
	std::vector<economic_accounting_item_reference> sorted_expected(expected.begin(),
									expected.end());
	std::sort(retained.begin(), retained.end(), less);
	std::sort(sorted_expected.begin(), sorted_expected.end(), less);
	for (size_t index = 0; index < retained.size(); ++index)
	{
		std::vector<uint8_t> actual_bytes, expected_bytes;
		if (flatfile_item_accounting_reference_encode(retained[index], &actual_bytes) !=
			    flatfile_item_accounting_status::ok ||
		    flatfile_item_accounting_reference_encode(sorted_expected[index],
							      &expected_bytes) !=
			    flatfile_item_accounting_status::ok ||
		    actual_bytes != expected_bytes)
			return flatfile_item_accounting_status::invalid;
	}
	return flatfile_item_accounting_status::ok;
}
catch (const std::bad_alloc &)
{
	return flatfile_item_accounting_status::capacity;
}

flatfile_item_accounting_status flatfile_item_accounting_reference_find_by_legacy(
	const std::string &root, const critical_operation_id &legacy_operation_id,
	uint16_t legacy_event_index, economic_accounting_item_reference *ref, std::string *error)
{
	const std::string dir = item_refs_directory(root);
	const std::string filename = bucket_filename(legacy_operation_id.bytes[0]);

	std::vector<uint8_t> existing;
	auto read_result = flatfile_read(dir, filename,
					 FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
					 &existing, error);
	if (read_result == flatfile_read_result::not_found)
		return flatfile_item_accounting_status::not_found;
	if (read_result != flatfile_read_result::ok)
		return read_status(read_result);
	if (!valid_bucket(existing, legacy_operation_id.bytes[0]))
	{
		if (error)
			*error = "invalid item accounting reference bucket";
		return flatfile_item_accounting_status::invalid;
	}

	const size_t record_size = FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES;
	const size_t total_records = existing.size() / record_size;

	bool found = false;
	economic_accounting_item_reference retained = {};
	for (size_t i = 0; i < total_records; ++i)
	{
		std::span<const uint8_t> chunk(existing.data() + i * record_size, record_size);
		economic_accounting_item_reference candidate = {};
		if (flatfile_item_accounting_reference_decode(chunk, &candidate) ==
		    flatfile_item_accounting_status::ok)
		{
			if (candidate.legacy_operation_id.bytes == legacy_operation_id.bytes &&
			    candidate.legacy_event_index == legacy_event_index)
			{
				if (found)
					return flatfile_item_accounting_status::invalid;
				retained = candidate;
				found = true;
			}
		}
	}
	if (found)
	{
		if (ref)
			*ref = retained;
		return flatfile_item_accounting_status::ok;
	}

	return flatfile_item_accounting_status::not_found;
}

flatfile_item_accounting_status flatfile_item_accounting_reference_find_by_item(
	const std::string &root, uint64_t item_uid, uint64_t after_revision,
	economic_accounting_item_reference *ref, std::string *error)
{
	const std::string dir = item_refs_directory(root);
	const size_t record_size = FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES;

	bool found = false;
	economic_accounting_item_reference retained = {};
	for (unsigned int b = 0; b < 256; ++b)
	{
		const std::string filename = bucket_filename(static_cast<uint8_t>(b));
		std::vector<uint8_t> existing;
		auto read_result = flatfile_read(
			dir, filename, FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
			&existing, error);
		if (read_result == flatfile_read_result::not_found)
			continue;
		if (read_result != flatfile_read_result::ok)
			return read_status(read_result);
		if (!valid_bucket(existing, static_cast<uint8_t>(b)))
		{
			if (error)
				*error = "invalid item accounting reference bucket";
			return flatfile_item_accounting_status::invalid;
		}

		const size_t total_records = existing.size() / record_size;
		for (size_t i = 0; i < total_records; ++i)
		{
			std::span<const uint8_t> chunk(existing.data() + i * record_size,
						       record_size);
			economic_accounting_item_reference candidate = {};
			if (flatfile_item_accounting_reference_decode(chunk, &candidate) ==
			    flatfile_item_accounting_status::ok)
			{
				if (candidate.item_uid == item_uid &&
				    candidate.after_revision == after_revision)
				{
					if (found)
						return flatfile_item_accounting_status::invalid;
					retained = candidate;
					found = true;
				}
			}
		}
	}
	if (found)
	{
		if (ref)
			*ref = retained;
		return flatfile_item_accounting_status::ok;
	}

	return flatfile_item_accounting_status::not_found;
}

flatfile_item_accounting_status flatfile_item_accounting_reference_find_by_operation(
	const std::string &root, const critical_operation_id &operation_id,
	std::vector<economic_accounting_item_reference> *refs, std::string *error)
{
	if (!refs)
		return flatfile_item_accounting_status::invalid;
	refs->clear();

	const std::string dir = item_refs_directory(root);
	const size_t record_size = FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES;

	for (unsigned int b = 0; b < 256; ++b)
	{
		const std::string filename = bucket_filename(static_cast<uint8_t>(b));
		std::vector<uint8_t> existing;
		auto read_result = flatfile_read(dir, filename, 64 * 1024 * 1024, &existing, error);
		if (read_result != flatfile_read_result::ok)
			continue;

		const size_t total_records = existing.size() / record_size;
		for (size_t i = 0; i < total_records; ++i)
		{
			std::span<const uint8_t> chunk(existing.data() + i * record_size,
						       record_size);
			economic_accounting_item_reference candidate = {};
			if (flatfile_item_accounting_reference_decode(chunk, &candidate) ==
			    flatfile_item_accounting_status::ok)
			{
				if (candidate.operation_id.bytes == operation_id.bytes)
				{
					refs->push_back(candidate);
				}
			}
		}
	}

	std::sort(refs->begin(), refs->end(),
		  [](const economic_accounting_item_reference &a,
		     const economic_accounting_item_reference &b)
		  { return a.line_index < b.line_index; });

	return flatfile_item_accounting_status::ok;
}

flatfile_item_accounting_status flatfile_item_accounting_reference_find_history(
	const std::string &root, uint64_t item_uid,
	std::vector<economic_accounting_item_reference> *history, std::string *error)
{
	if (!history || item_uid == 0)
		return flatfile_item_accounting_status::invalid;

	history->clear();

	const std::string dir = item_refs_directory(root);
	const size_t record_size = FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES;

	for (unsigned int b = 0; b < 256; ++b)
	{
		const std::string filename = bucket_filename(static_cast<uint8_t>(b));
		std::vector<uint8_t> existing;
		auto read_result = flatfile_read(dir, filename, 64 * 1024 * 1024, &existing, error);
		if (read_result != flatfile_read_result::ok)
			continue;

		const size_t total_records = existing.size() / record_size;
		for (size_t i = 0; i < total_records; ++i)
		{
			std::span<const uint8_t> chunk(existing.data() + i * record_size,
						       record_size);
			economic_accounting_item_reference candidate = {};
			if (flatfile_item_accounting_reference_decode(chunk, &candidate) ==
			    flatfile_item_accounting_status::ok)
			{
				if (candidate.item_uid == item_uid)
				{
					history->push_back(candidate);
				}
			}
		}
	}

	std::sort(history->begin(), history->end(),
		  [](const economic_accounting_item_reference &a,
		     const economic_accounting_item_reference &b)
		  { return a.after_revision < b.after_revision; });

	return flatfile_item_accounting_status::ok;
}

flatfile_item_accounting_status flatfile_item_accounting_reference_verify_and_repair(
	const std::string &root, flatfile_accounting_audit_report *report, std::string *error)
{
	flatfile_accounting_audit_report local_report = {};
	const std::string dir = item_refs_directory(root);
	const size_t record_size = FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES;

	for (unsigned int b = 0; b < 256; ++b)
	{
		const uint8_t bucket = static_cast<uint8_t>(b);
		const std::string filename = bucket_filename(bucket);
		std::vector<uint8_t> existing;
		auto read_result = flatfile_read(dir, filename, 64 * 1024 * 1024, &existing, error);
		if (read_result == flatfile_read_result::not_found)
			continue;
		if (read_result != flatfile_read_result::ok)
		{
			++local_report.corrupted_buckets_quarantined;
			continue;
		}

		++local_report.total_buckets_scanned;
		const size_t trailing_bytes = existing.size() % record_size;
		const size_t valid_bytes = existing.size() - trailing_bytes;
		const size_t record_count = valid_bytes / record_size;

		bool bucket_records_valid = true;
		for (size_t i = 0; i < record_count; ++i)
		{
			std::span<const uint8_t> chunk(existing.data() + i * record_size,
						       record_size);
			economic_accounting_item_reference candidate = {};
			if (flatfile_item_accounting_reference_decode(chunk, &candidate) !=
				    flatfile_item_accounting_status::ok ||
			    candidate.legacy_operation_id.bytes[0] != bucket)
			{
				bucket_records_valid = false;
				break;
			}
		}

		if (!bucket_records_valid)
		{
			const std::string quarantine_name = filename + ".corrupt";
			flatfile_atomic_write(dir, quarantine_name, existing, nullptr);
			std::vector<uint8_t> empty;
			flatfile_atomic_write(dir, filename, empty, nullptr);
			++local_report.corrupted_buckets_quarantined;
			continue;
		}

		if (trailing_bytes > 0)
		{
			existing.resize(valid_bytes);
			if (flatfile_atomic_write(dir, filename, existing, error))
			{
				local_report.partial_trailing_bytes_truncated += trailing_bytes;
			}
		}

		local_report.valid_records_count += record_count;
	}

	if (report)
		*report = local_report;
	return flatfile_item_accounting_status::ok;
}

#include <cerrno>
#include <new>

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
bool reference_bound_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
bool reference_bound_rows(size_t &total, size_t count, size_t width) noexcept
{
	return count <= SIZE_MAX / width && reference_bound_add(total, count * width);
}
struct reference_bound_less
{
	bool operator()(const economic_accounting_item_reference &left,
			const economic_accounting_item_reference &right) const noexcept
	{
		return left.legacy_event_index < right.legacy_event_index;
	}
};
struct reference_operation_workspace
{
	reference_operation_workspace(size_t directory_size)
		: directory(directory_size, '\0')
	{
	}
	std::string directory, filename;
	std::vector<uint8_t> existing;
	std::vector<economic_accounting_item_reference> retained, sorted_expected;
	economic_accounting_item_reference reference{};
	reference_bound_less less;
};
// The original decoder owns its reader, complete decoded DTO and input span,
// two retained identity spans and transient read_number()/take() spans. Its
// scalar identity validator allocates nothing. The original bucket validator
// additionally owns its full input span, selected record span and output DTO.
constexpr size_t reference_decode_working = sizeof(reader) +
					    sizeof(economic_accounting_item_reference) +
					    sizeof(std::span<const uint8_t>) * 5;
constexpr size_t reference_bucket_working = sizeof(economic_accounting_item_reference) +
					    sizeof(std::span<const uint8_t>) * 2 +
					    reference_decode_working;
#endif
}

flatfile_item_accounting_status flatfile_item_accounting_reference_verify_operation_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &legacy_operation_id,
	const std::span<const economic_accounting_item_reference> &expected,
	flatfile_scratch_reserve_fn reserve, void *context, size_t outer) noexcept
{
	using status = flatfile_item_accounting_status;
	if (root.empty() || critical_operation_id_is_zero(legacy_operation_id) ||
	    !lock.matches(root) || !reserve)
		return status::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)expected;
	(void)context;
	(void)outer;
	errno = ENOTSUP;
	return status::io_error;
#else
	size_t directory_size = root.size(), fixed = outer;
	if (!reference_bound_add(directory_size, sizeof("/accounting/item_references") - 1) ||
	    !reference_bound_add(fixed, sizeof(reference_operation_workspace)) ||
	    (directory_size > 15 &&
	     (directory_size == SIZE_MAX || !reference_bound_add(fixed, directory_size + 1))))
		return status::capacity;
	size_t filename_peak = fixed;
	if (!reference_bound_add(filename_peak, sizeof(char[16])) ||
	    !reserve(filename_peak, context))
		return status::capacity;
	try
	{
		const uint8_t bucket = legacy_operation_id.bytes[0];
		reference_operation_workspace work(directory_size);
		std::copy(root.begin(), root.end(), work.directory.begin());
		std::copy_n("/accounting/item_references",
			    sizeof("/accounting/item_references") - 1,
			    work.directory.begin() + root.size());
		{
			char filename[16] = {};
			std::snprintf(filename, sizeof(filename), "%02x.bin", bucket);
			work.filename.assign(filename, 6);
		}
		// Canonical filename is inline; formatting array dies before file stat.
		const auto loaded =
			flatfile_read_bounded(work.directory, work.filename,
					      FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
					      &work.existing, reserve, context, fixed);
		if (loaded == flatfile_read_result::not_found)
			return expected.empty() ? status::ok : status::not_found;
		if (loaded != flatfile_read_result::ok)
			return read_status(loaded);
		size_t live = fixed;
		if (!reference_bound_add(live, work.existing.capacity()))
			return status::capacity;
		size_t bucket_peak = live;
		if (!reference_bound_add(bucket_peak, reference_bucket_working) ||
		    !reserve(bucket_peak, context))
			return status::capacity;
		// Preserve complete original first pass, including every other operation.
		if (!valid_bucket(work.existing, bucket))
			return status::invalid;
		for (size_t offset = 0; offset < work.existing.size();
		     offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
		{
			size_t decode_peak = live;
			if (!reference_bound_rows(decode_peak, work.retained.capacity(),
						  sizeof(economic_accounting_item_reference)) ||
			    !reference_bound_add(decode_peak,
						 reference_decode_working +
							 sizeof(std::span<const uint8_t>)) ||
			    !reserve(decode_peak, context))
				return status::capacity;
			if (flatfile_item_accounting_reference_decode(
				    std::span<const uint8_t>(work.existing)
					    .subspan(
						    offset,
						    FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES),
				    &work.reference) != status::ok)
				return status::invalid;
			if (work.reference.legacy_operation_id.bytes == legacy_operation_id.bytes)
			{
				if (work.retained.size() == work.retained.capacity())
				{
					const size_t size = work.retained.size();
					if (size > SIZE_MAX / 2)
						return status::capacity;
					const size_t next_capacity = size ? size * 2 : 1;
					size_t growth_peak = live;
					if (!reference_bound_rows(
						    growth_peak, work.retained.capacity(),
						    sizeof(economic_accounting_item_reference)) ||
					    !reference_bound_rows(
						    growth_peak, next_capacity,
						    sizeof(economic_accounting_item_reference)) ||
					    !reserve(growth_peak, context))
						return status::capacity;
				}
				work.retained.push_back(work.reference);
			}
		}
		if (work.retained.size() != expected.size())
			return status::invalid;
		if (!reference_bound_rows(live, work.retained.capacity(),
					  sizeof(economic_accounting_item_reference)))
			return status::capacity;
		size_t copied_live = live;
		if (!reference_bound_rows(copied_live, expected.size(),
					  sizeof(economic_accounting_item_reference)) ||
		    !reference_bound_add(copied_live,
					 sizeof(std::vector<economic_accounting_item_reference>)) ||
		    !reserve(copied_live, context))
			return status::capacity;
		{
			std::vector<economic_accounting_item_reference> sorted_expected(
				expected.begin(), expected.end());
			work.sorted_expected = std::move(sorted_expected);
		}
		if (!reference_bound_rows(live, work.sorted_expected.capacity(),
					  sizeof(economic_accounting_item_reference)))
			return status::capacity;
		std::sort(work.retained.begin(), work.retained.end(), work.less);
		std::sort(work.sorted_expected.begin(), work.sorted_expected.end(), work.less);
		for (size_t index = 0; index < work.retained.size(); ++index)
		{
			size_t encode_live = live;
			if (!reference_bound_add(encode_live, sizeof(std::vector<uint8_t>) * 2))
				return status::capacity;
			size_t first_peak = encode_live;
			if (!reference_bound_add(first_peak,
						 FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES +
							 sizeof(std::span<const uint8_t>)) ||
			    !reserve(first_peak, context))
				return status::capacity;
			std::vector<uint8_t> actual_bytes, expected_bytes;
			if (flatfile_item_accounting_reference_encode(work.retained[index],
								      &actual_bytes) != status::ok)
				return status::invalid;
			size_t second_peak = encode_live;
			if (!reference_bound_add(second_peak, actual_bytes.capacity()) ||
			    !reference_bound_add(second_peak,
						 FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES +
							 sizeof(std::span<const uint8_t>)) ||
			    !reserve(second_peak, context))
				return status::capacity;
			if (flatfile_item_accounting_reference_encode(
				    work.sorted_expected[index], &expected_bytes) != status::ok ||
			    actual_bytes != expected_bytes)
				return status::invalid;
		}
		return status::ok;
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

flatfile_item_accounting_status
flatfile_native_mobile_birth_ordinary_reference_history_storage::verify_initial_absence_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_reference_absence *output,
	std::string *error) noexcept
{
	using status = flatfile_item_accounting_status;
	if (root.empty() || !output || !lock.matches(root))
		return status::invalid;
	try
	{
		if (!native_mobile_birth_cash_role_recovery_initial(original))
			return status::invalid;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		economic_frozen_intent intent;
		if (native_mobile_birth_cash_role_command_decode(original.command, &image, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    economic_intent_decode(original.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(original.command, intent) !=
			    economic_accounting_error::ok ||
		    !intent.admission.metadata.source_event)
			return status::invalid;
		std::vector<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &literal : image.items)
			born.push_back(literal.object_uid);
		std::sort(born.begin(), born.end());
		if (std::adjacent_find(born.begin(), born.end()) != born.end() ||
		    (!born.empty() && !born.front()))
			return status::invalid;

		// Match the ORIGINAL SQL primary/unique keys, including duplicates that
		// cross legacy bucket boundaries. UID/after_revision is only a nonunique
		// SQL history index; do not invent a new uniqueness rule for it.
		using reference_key = std::pair<std::array<uint8_t, 16>, uint16_t>;
		std::set<reference_key> operation_lines, legacy_events;
		const std::string directory = item_refs_directory(root);
		flatfile_native_mobile_birth_ordinary_reference_absence observed;
		observed.born_uids = born.size();
		for (unsigned int bucket = 0; bucket < 256; ++bucket)
		{
			const auto shard = static_cast<uint8_t>(bucket);
			const std::string filename = bucket_filename(shard);
			std::vector<uint8_t> existing;
			const auto loaded =
				flatfile_read(directory, filename,
					      FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
					      &existing, error);
			if (loaded == flatfile_read_result::not_found)
			{
				++observed.missing_buckets;
				++observed.buckets_verified;
				continue;
			}
			if (loaded != flatfile_read_result::ok)
				return read_status(loaded);
			if (existing.size() % FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
				return status::invalid;
			for (size_t offset = 0; offset < existing.size();
			     offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
			{
				const auto bytes = std::span<const uint8_t>(existing).subspan(
					offset, FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
				economic_accounting_item_reference reference{};
				const auto decoded = flatfile_item_accounting_reference_decode(
					bytes, &reference);
				if (decoded != status::ok)
					return decoded;
				if (reference.legacy_operation_id.bytes[0] != shard)
					return status::invalid;
				std::vector<uint8_t> canonical;
				const auto encoded = flatfile_item_accounting_reference_encode(
					reference, &canonical);
				if (encoded != status::ok)
					return encoded;
				if (canonical.size() != bytes.size() ||
				    !std::equal(canonical.begin(), canonical.end(),
						bytes.begin()) ||
				    !operation_lines
					     .emplace(reference.operation_id.bytes,
						      reference.line_index)
					     .second ||
				    !legacy_events
					     .emplace(reference.legacy_operation_id.bytes,
						      reference.legacy_event_index)
					     .second)
					return status::invalid;
				if (std::binary_search(born.begin(), born.end(),
						       reference.item_uid) ||
				    reference.operation_id.bytes ==
					    original.command.operation_id.bytes ||
				    reference.legacy_operation_id.bytes ==
					    original.command.operation_id.bytes)
					return status::already_exists;
				++observed.retained_records;
			}
			++observed.buckets_verified;
		}
		if (observed.buckets_verified != 256 || !lock.matches(root))
			return status::invalid;
		// No identity/activity filter exists here. A retired/dead owner's exact
		// retained reference prevents INITIAL UID reuse just as the original SQL
		// all-history SELECT does. Other history/physical namespaces stay separate.
		*output = observed;
		return status::ok;
	}
	catch (const std::bad_alloc &)
	{
		return status::capacity;
	}
	catch (...)
	{
		return status::io_error;
	}
}

flatfile_item_accounting_status flatfile_native_mobile_birth_ordinary_reference_history_storage::
	verify_initial_quarantine_absence_locked(
		const std::string &root, const flatfile_authority_lock &lock,
		const critical_native_recovery_envelope &original,
		flatfile_native_mobile_birth_ordinary_reference_quarantine_absence *output,
		std::string *error) noexcept
{
	using status = flatfile_item_accounting_status;
	if (root.empty() || !output || !lock.matches(root))
		return status::invalid;
	try
	{
		if (!native_mobile_birth_cash_role_recovery_initial(original))
			return status::invalid;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		economic_frozen_intent intent;
		if (native_mobile_birth_cash_role_command_decode(original.command, &image, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    economic_intent_decode(original.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(original.command, intent) !=
			    economic_accounting_error::ok ||
		    !intent.admission.metadata.source_event)
			return status::invalid;
		std::vector<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &literal : image.items)
			born.push_back(literal.object_uid);
		std::sort(born.begin(), born.end());
		if (std::adjacent_find(born.begin(), born.end()) != born.end() ||
		    (!born.empty() && !born.front()))
			return status::invalid;

		// Quarantine files are retained evidence copies, not a second SQL table.
		// Census original key identities without imposing SQL insertion uniqueness
		// on those copies. Every decoded retained row still participates in absence.
		using key = std::pair<std::array<uint8_t, 16>, uint16_t>;
		std::set<key> operation_lines, legacy_events;
		const std::string directory = item_refs_directory(root);
		flatfile_native_mobile_birth_ordinary_reference_quarantine_absence observed;
		observed.born_uids = born.size();
		for (unsigned int bucket = 0; bucket < 256; ++bucket)
		{
			const auto shard = static_cast<uint8_t>(bucket);
			const std::string filename = bucket_filename(shard) + ".corrupt";
			std::vector<uint8_t> retained;
			const auto loaded =
				flatfile_read(directory, filename,
					      FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
					      &retained, error);
			if (loaded == flatfile_read_result::not_found)
			{
				++observed.missing_files;
				++observed.buckets_verified;
				continue;
			}
			if (loaded != flatfile_read_result::ok)
				return read_status(loaded);
			if (retained.size() % FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
			{
				if (error)
					*error =
						"retained reference quarantine has unknown partial-record evidence";
				return status::invalid;
			}
			for (size_t offset = 0; offset < retained.size();
			     offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
			{
				const auto bytes = std::span<const uint8_t>(retained).subspan(
					offset, FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
				economic_accounting_item_reference reference{};
				if (flatfile_item_accounting_reference_decode(bytes, &reference) !=
					    status::ok ||
				    reference.legacy_operation_id.bytes[0] != shard)
				{
					if (error)
						*error =
							"retained reference quarantine cannot authenticate record identity";
					return status::invalid;
				}
				std::vector<uint8_t> canonical;
				const auto encoded = flatfile_item_accounting_reference_encode(
					reference, &canonical);
				if (encoded != status::ok)
					return encoded;
				if (canonical.size() != bytes.size() ||
				    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
				{
					if (error)
						*error =
							"retained reference quarantine is not canonical evidence";
					return status::invalid;
				}
				if (std::binary_search(born.begin(), born.end(),
						       reference.item_uid) ||
				    reference.operation_id.bytes ==
					    original.command.operation_id.bytes ||
				    reference.legacy_operation_id.bytes ==
					    original.command.operation_id.bytes)
				{
					if (error)
						*error =
							"ordinary birth conflicts with retained reference quarantine evidence";
					return status::already_exists;
				}
				operation_lines.emplace(reference.operation_id.bytes,
							reference.line_index);
				legacy_events.emplace(reference.legacy_operation_id.bytes,
						      reference.legacy_event_index);
				++observed.retained_records;
			}
			++observed.buckets_verified;
		}
		if (observed.buckets_verified != 256 || !lock.matches(root))
			return status::invalid;
		observed.unique_operation_lines = operation_lines.size();
		observed.unique_legacy_events = legacy_events.size();
		// Strong scalar output: this describes currently retained quarantine
		// bytes only. Original repair can overwrite a quarantine or truncate
		// trailing canonical data without retaining it; no historical-completeness
		// or CURRENT law follows from these counts.
		*output = observed;
		return status::ok;
	}
	catch (const std::bad_alloc &)
	{
		return status::capacity;
	}
	catch (...)
	{
		return status::io_error;
	}
}

flatfile_item_accounting_status
flatfile_native_mobile_birth_ordinary_reference_history_storage::verify_current_operation_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation,
	std::span<const economic_accounting_item_reference> expected,
	flatfile_native_mobile_birth_ordinary_reference_current *output,
	std::string *error) noexcept
{
	using status = flatfile_item_accounting_status;
	if (root.empty() || !output || !lock.matches(root) ||
	    critical_operation_id_is_zero(operation))
		return status::invalid;
	try
	{
		std::vector<economic_accounting_item_reference> sorted(expected.begin(),
								       expected.end());
		std::sort(sorted.begin(), sorted.end(),
			  [](const auto &a, const auto &b) { return a.line_index < b.line_index; });
		for (size_t index = 0; index < sorted.size(); ++index)
		{
			const auto &row = sorted[index];
			if (!economic_accounting_item_reference_validate(row) ||
			    row.operation_id.bytes != operation.bytes ||
			    row.legacy_operation_id.bytes != operation.bytes ||
			    (index && sorted[index - 1].line_index == row.line_index))
				return status::invalid;
		}
		using key = std::pair<std::array<uint8_t, 16>, uint16_t>;
		std::set<key> operation_lines, legacy_events;
		std::set<uint16_t> matched;
		const std::string directory = item_refs_directory(root);
		flatfile_native_mobile_birth_ordinary_reference_current observed;
		bool selected_missing = false;
		for (unsigned int bucket = 0; bucket < 256; ++bucket)
		{
			const auto shard = static_cast<uint8_t>(bucket);
			std::vector<uint8_t> existing;
			const auto loaded =
				flatfile_read(directory, bucket_filename(shard),
					      FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
					      &existing, error);
			if (loaded == flatfile_read_result::not_found)
			{
				selected_missing |= shard == operation.bytes[0];
				++observed.missing_buckets;
				++observed.buckets_verified;
				continue;
			}
			if (loaded != flatfile_read_result::ok)
				return read_status(loaded);
			if (existing.size() % FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
				return status::invalid;
			for (size_t offset = 0; offset < existing.size();
			     offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
			{
				const auto bytes = std::span<const uint8_t>(existing).subspan(
					offset, FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
				economic_accounting_item_reference row;
				const auto decoded =
					flatfile_item_accounting_reference_decode(bytes, &row);
				if (decoded != status::ok)
					return decoded;
				if (row.legacy_operation_id.bytes[0] != shard)
					return status::invalid;
				std::vector<uint8_t> canonical;
				auto encoded =
					flatfile_item_accounting_reference_encode(row, &canonical);
				if (encoded != status::ok)
					return encoded;
				if (canonical.size() != bytes.size() ||
				    !std::equal(canonical.begin(), canonical.end(),
						bytes.begin()) ||
				    !operation_lines.emplace(row.operation_id.bytes, row.line_index)
					     .second ||
				    !legacy_events
					     .emplace(row.legacy_operation_id.bytes,
						      row.legacy_event_index)
					     .second)
					return status::invalid;
				const bool root_match = row.operation_id.bytes == operation.bytes;
				const bool legacy_match = row.legacy_operation_id.bytes ==
							  operation.bytes;
				if (root_match || legacy_match)
				{
					// A foreign legacy identity can place an extra root row in
					// any shard. Both root and legacy must match the full typed
					// ordinary-birth expectation, even for a zero-stock birth.
					if (!root_match || !legacy_match)
						return status::invalid;
					const auto found = std::lower_bound(
						sorted.begin(), sorted.end(), row.line_index,
						[](const auto &value, uint16_t line)
						{ return value.line_index < line; });
					if (found == sorted.end() ||
					    found->line_index != row.line_index ||
					    !matched.insert(row.line_index).second)
						return status::invalid;
					std::vector<uint8_t> expected_bytes;
					encoded = flatfile_item_accounting_reference_encode(
						*found, &expected_bytes);
					if (encoded != status::ok)
						return encoded;
					if (canonical != expected_bytes)
						return status::invalid;
					++observed.root_records;
					++observed.legacy_records;
				}
				++observed.retained_records;
			}
			++observed.buckets_verified;
		}
		if (observed.buckets_verified != 256 || !lock.matches(root))
			return status::invalid;
		if (observed.root_records != expected.size() ||
		    observed.legacy_records != expected.size() || matched.size() != expected.size())
			return selected_missing && !expected.empty() ? status::not_found :
								       status::invalid;
		*output = observed;
		return status::ok;
	}
	catch (const std::bad_alloc &)
	{
		return status::capacity;
	}
	catch (...)
	{
		return status::io_error;
	}
}

flatfile_item_accounting_status
flatfile_native_mobile_birth_ordinary_reference_stage_storage::stage_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &legacy_operation_id,
	std::span<const economic_accounting_item_reference> references,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !operations ||
	    critical_operation_id_is_zero(legacy_operation_id))
		return flatfile_item_accounting_status::invalid;
	const critical_operation_id &operation_id = legacy_operation_id;
	const uint8_t bucket = operation_id.bytes[0];

	std::vector<uint8_t> addition;
	addition.reserve(references.size() * FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
	std::vector<uint16_t> event_indexes;
	event_indexes.reserve(references.size());
	for (const auto &reference : references)
	{
		if (reference.legacy_operation_id.bytes != operation_id.bytes)
			return flatfile_item_accounting_status::invalid;
		std::vector<uint8_t> encoded;
		const auto status = flatfile_item_accounting_reference_encode(reference, &encoded);
		if (status != flatfile_item_accounting_status::ok)
			return status;
		if (std::find(event_indexes.begin(), event_indexes.end(),
			      reference.legacy_event_index) != event_indexes.end())
			return flatfile_item_accounting_status::invalid;
		event_indexes.push_back(reference.legacy_event_index);
		addition.insert(addition.end(), encoded.begin(), encoded.end());
	}

	const std::string dir = item_refs_directory(root);
	// Authenticate the ACTUAL established private namespace via original FD IO.
	// Missing directory/security/read failure is not empty, even with zero refs
	// or an already staged image. No mkdir/chmod/recovery belongs to this leaf.
	{
		std::vector<uint8_t> namespace_probe;
		const auto namespace_status =
			flatfile_read(dir, bucket_filename(bucket),
				      FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
				      &namespace_probe, error);
		if (namespace_status != flatfile_read_result::ok &&
		    namespace_status != flatfile_read_result::not_found)
			return read_status(namespace_status);
	}
	const std::string filename = bucket_filename(bucket);
	std::vector<uint8_t> existing;
	size_t staged_index = operations->size();
	for (size_t index = 0; index < operations->size(); ++index)
		if ((*operations)[index].store ==
			    flatfile_authority_store::item_accounting_references &&
		    (*operations)[index].filename == filename)
		{
			if (staged_index != operations->size() ||
			    (*operations)[index].kind != flatfile_authority_operation_kind::write)
				return flatfile_item_accounting_status::invalid;
			staged_index = index;
		}
	if (staged_index < operations->size())
		existing = (*operations)[staged_index].bytes;
	else
	{
		if (!references.empty() &&
		    operations->size() >= flatfile_authority_transaction_maximum_operations)
			return flatfile_item_accounting_status::capacity;
		const auto read_result = flatfile_read(
			dir, filename, FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
			&existing, error);
		if (read_result != flatfile_read_result::ok &&
		    read_result != flatfile_read_result::not_found)
			return read_status(read_result);
	}
	if (!valid_bucket(existing, bucket))
		return flatfile_item_accounting_status::invalid;
	for (size_t offset = 0; offset < existing.size();
	     offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
	{
		economic_accounting_item_reference retained = {};
		if (flatfile_item_accounting_reference_decode(
			    std::span<const uint8_t>(existing).subspan(
				    offset, FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES),
			    &retained) != flatfile_item_accounting_status::ok)
			return flatfile_item_accounting_status::invalid;
		if (retained.legacy_operation_id.bytes == operation_id.bytes)
			return flatfile_item_accounting_status::already_exists;
	}
	if (addition.size() > FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES - existing.size())
		return flatfile_item_accounting_status::capacity;
	if (references.empty())
		return flatfile_item_accounting_status::ok;
	if (addition.size() > flatfile_authority_transaction_maximum_bytes - existing.size())
		return flatfile_item_accounting_status::capacity;
	existing.insert(existing.end(), addition.begin(), addition.end());
	auto candidate = *operations;
	if (staged_index < candidate.size())
		candidate[staged_index].bytes = std::move(existing);
	else
		candidate.push_back({ flatfile_authority_store::item_accounting_references,
				      flatfile_authority_operation_kind::write, filename,
				      std::move(existing) });
	if (!lock.matches(root))
		return flatfile_item_accounting_status::invalid;
	*operations = std::move(candidate);
	return flatfile_item_accounting_status::ok;
}
catch (const std::bad_alloc &)
{
	return flatfile_item_accounting_status::capacity;
}

namespace
{
#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&     \
	!defined(_GLIBCXX_DEBUG)
using reference_current_key = std::pair<std::array<uint8_t, 16>, uint16_t>;
using reference_current_key_node = std::_Rb_tree_node<reference_current_key>;
using reference_current_line_node = std::_Rb_tree_node<uint16_t>;
struct reference_current_line_less
{
	bool operator()(const economic_accounting_item_reference &left,
			const economic_accounting_item_reference &right) const noexcept
	{
		return left.line_index < right.line_index;
	}
	bool operator()(const economic_accounting_item_reference &left,
			uint16_t right) const noexcept
	{
		return left.line_index < right;
	}
};
struct reference_current_workspace
{
	std::vector<economic_accounting_item_reference> sorted;
	std::set<reference_current_key> operation_lines, legacy_events;
	std::set<uint16_t> matched;
	std::string directory;
	flatfile_native_mobile_birth_ordinary_reference_current observed{};
	reference_current_line_less less;
	size_t directory_size = 0, fixed = 0, live = 0, peak = 0, index = 0;
	unsigned int bucket = 0;
	bool selected_missing = false;
};
struct reference_current_bucket_workspace
{
	std::string filename;
	std::vector<uint8_t> existing;
	char formatted[16]{};
	size_t offset = 0;
	uint8_t shard = 0;
	flatfile_read_result loaded = flatfile_read_result::invalid;
};
struct reference_current_row_workspace
{
	std::span<const uint8_t> bytes;
	economic_accounting_item_reference row{};
	std::vector<uint8_t> canonical, expected_bytes;
	std::vector<economic_accounting_item_reference>::const_iterator found;
	flatfile_item_accounting_status result = flatfile_item_accounting_status::invalid;
	bool root_match = false, legacy_match = false;
};

// Named parameters/return carriers for this leaf, the census, checked sums,
// and reserve callback. This is a source storage profile, not emitted frames.
constexpr size_t reference_current_leaf_frames =
	sizeof(void *) * 6 + sizeof(std::span<const economic_accounting_item_reference>) +
	sizeof(size_t) + sizeof(flatfile_item_accounting_status);
constexpr size_t reference_current_census_frames =
	sizeof(void *) * 8 + sizeof(size_t) * 5 + sizeof(bool) * 4;

// Original fixed-record codec source: decoder parameters plus its complete
// reader, returned/taken spans, DTO, identity spans and decoded scalars. Nested
// read_number/take source carriers and the allocation-free original validator
// are charged together. Original bodies remain selected, byte-exact.
constexpr size_t reference_current_validator_frames =
	sizeof(void *) * 5 + sizeof(uint8_t) + sizeof(bool) * 2;
constexpr size_t reference_current_decode_frames =
	sizeof(reader) + sizeof(economic_accounting_item_reference) +
	sizeof(std::span<const uint8_t>) * 6 + sizeof(void *) * 5 + sizeof(size_t) * 6 +
	sizeof(uint64_t) * 5 + sizeof(bool) * 3 + sizeof(uint32_t) + sizeof(uint16_t) * 3 +
	sizeof(flatfile_item_accounting_status) + reference_current_validator_frames;
// Encoder parameters plus raw()/number() parameter and loop carriers. The
// larger raw branch includes its span and return carrier; both branches are
// reserved, conservatively overlapping, before the original reserve(66).
constexpr size_t reference_current_encode_frames =
	sizeof(void *) * 6 + sizeof(std::span<const uint8_t>) + sizeof(size_t) * 2 +
	sizeof(uint64_t) + sizeof(flatfile_item_accounting_status) +
	reference_current_validator_frames;

// Secure read's stat and fresh candidate vector are admitted by the genuine
// flatfile_read_bounded provider. Its named parameters/scalars and read_all /
// private_directory carriers are added here, not guessed as an opaque CB.
// Library, syscall and emitted-frame qualification remains separate.
constexpr size_t reference_current_read_frames = sizeof(void *) * 7 + sizeof(size_t) * 6 +
						 sizeof(int) * 5 + sizeof(ssize_t) +
						 sizeof(flatfile_read_result) + sizeof(bool) * 2;

bool reference_current_census(reference_current_workspace &work,
			      const reference_current_bucket_workspace *bucket,
			      const reference_current_row_workspace *row, size_t extra,
			      flatfile_scratch_reserve_fn reserve, void *context) noexcept
{
	work.live = work.fixed;
	if (!reference_bound_rows(work.live, work.sorted.capacity(),
				  sizeof(economic_accounting_item_reference)) ||
	    !reference_bound_rows(work.live, work.operation_lines.size(),
				  sizeof(reference_current_key_node)) ||
	    !reference_bound_rows(work.live, work.legacy_events.size(),
				  sizeof(reference_current_key_node)) ||
	    !reference_bound_rows(work.live, work.matched.size(),
				  sizeof(reference_current_line_node)) ||
	    (work.directory.capacity() > 15 &&
	     (work.directory.capacity() == SIZE_MAX ||
	      !reference_bound_add(work.live, work.directory.capacity() + 1))))
		return false;
	if (bucket && (!reference_bound_add(work.live, sizeof(*bucket)) ||
		       !reference_bound_add(work.live, bucket->existing.capacity()) ||
		       (bucket->filename.capacity() > 15 &&
			(bucket->filename.capacity() == SIZE_MAX ||
			 !reference_bound_add(work.live, bucket->filename.capacity() + 1)))))
		return false;
	if (row && (!reference_bound_add(work.live, sizeof(*row)) ||
		    !reference_bound_add(work.live, row->canonical.capacity()) ||
		    !reference_bound_add(work.live, row->expected_bytes.capacity())))
		return false;
	work.peak = work.live;
	return reference_bound_add(work.peak, reference_current_census_frames) &&
	       reference_bound_add(work.peak, extra) && reserve(work.peak, context);
}
#endif
} // namespace

flatfile_item_accounting_status flatfile_native_mobile_birth_ordinary_reference_history_storage::
	verify_current_operation_locked_bounded(
		const std::string &root, const flatfile_authority_lock &lock,
		const critical_operation_id &operation,
		std::span<const economic_accounting_item_reference> expected,
		flatfile_native_mobile_birth_ordinary_reference_current *output,
		flatfile_scratch_reserve_fn reserve, void *context, size_t outer) noexcept
{
	using status = flatfile_item_accounting_status;
	if (root.empty() || !output || !reserve)
		return status::invalid;
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) || \
	!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 ||          \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)expected;
	(void)context;
	(void)outer;
	errno = ENOTSUP;
	return status::io_error;
#else
	// Admission precedes construction of every owned container, including the
	// empty forest. Caller retains actual root/lock/expected backing in outer.
	size_t first = outer;
	if (!reference_bound_add(first, reference_current_leaf_frames +
						sizeof(reference_current_workspace) +
						sizeof(size_t) + reference_current_census_frames) ||
	    !reserve(first, context))
		return status::capacity;
	if (!lock.matches(root) || critical_operation_id_is_zero(operation))
		return status::invalid;
	try
	{
		reference_current_workspace work;
		work.fixed = first - reference_current_census_frames;
		work.peak = sizeof(std::vector<economic_accounting_item_reference>);
		if (!reference_bound_rows(work.peak, expected.size(),
					  sizeof(economic_accounting_item_reference)) ||
		    !reference_current_census(work, nullptr, nullptr, work.peak, reserve, context))
			return status::capacity;
		{
			std::vector<economic_accounting_item_reference> sorted(expected.begin(),
									       expected.end());
			work.sorted = std::move(sorted);
		}
		if (!reference_current_census(work, nullptr, nullptr, 0, reserve, context))
			return status::capacity;
		std::sort(work.sorted.begin(), work.sorted.end(), work.less);
		for (work.index = 0; work.index < work.sorted.size(); ++work.index)
		{
			if (!reference_current_census(work, nullptr, nullptr,
						      reference_current_validator_frames, reserve,
						      context))
				return status::capacity;
			if (!economic_accounting_item_reference_validate(work.sorted[work.index]) ||
			    work.sorted[work.index].operation_id.bytes != operation.bytes ||
			    work.sorted[work.index].legacy_operation_id.bytes != operation.bytes ||
			    (work.index && work.sorted[work.index - 1].line_index ==
						   work.sorted[work.index].line_index))
				return status::invalid;
		}
		work.directory_size = root.size();
		if (!reference_bound_add(work.directory_size,
					 sizeof("/accounting/item_references") - 1))
			return status::capacity;
		work.peak = sizeof(std::string);
		if (work.directory_size > 15 &&
		    (work.directory_size == SIZE_MAX ||
		     !reference_bound_add(work.peak, work.directory_size + 1)))
			return status::capacity;
		if (!reference_current_census(work, nullptr, nullptr, work.peak, reserve, context))
			return status::capacity;
		{
			std::string directory(work.directory_size, '\0');
			std::copy(root.begin(), root.end(), directory.begin());
			std::copy_n("/accounting/item_references",
				    sizeof("/accounting/item_references") - 1,
				    directory.begin() + root.size());
			work.directory = std::move(directory);
		}
		for (work.bucket = 0; work.bucket < 256; ++work.bucket)
		{
			if (!reference_current_census(work, nullptr, nullptr,
						      sizeof(reference_current_bucket_workspace),
						      reserve, context))
				return status::capacity;
			reference_current_bucket_workspace bucket;
			bucket.shard = static_cast<uint8_t>(work.bucket);
			std::snprintf(bucket.formatted, sizeof(bucket.formatted), "%02x.bin",
				      bucket.shard);
			bucket.filename.assign(bucket.formatted, 6);
			if (!reference_current_census(work, &bucket, nullptr,
						      reference_current_read_frames, reserve,
						      context))
				return status::capacity;
			bucket.loaded = flatfile_read_bounded(
				work.directory, bucket.filename,
				FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES,
				&bucket.existing, reserve, context, work.peak);
			if (bucket.loaded == flatfile_read_result::not_found)
			{
				work.selected_missing |= bucket.shard == operation.bytes[0];
				++work.observed.missing_buckets;
				++work.observed.buckets_verified;
				continue;
			}
			if (bucket.loaded != flatfile_read_result::ok)
				return read_status(bucket.loaded);
			if (bucket.existing.size() %
			    FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
				return status::invalid;
			for (bucket.offset = 0; bucket.offset < bucket.existing.size();
			     bucket.offset += FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES)
			{
				if (!reference_current_census(
					    work, &bucket, nullptr,
					    sizeof(reference_current_row_workspace), reserve,
					    context))
					return status::capacity;
				reference_current_row_workspace row;
				row.bytes =
					std::span<const uint8_t>(bucket.existing)
						.subspan(
							bucket.offset,
							FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
				if (!reference_current_census(work, &bucket, &row,
							      reference_current_decode_frames,
							      reserve, context))
					return status::capacity;
				row.result = flatfile_item_accounting_reference_decode(row.bytes,
										       &row.row);
				if (row.result != status::ok)
					return row.result;
				if (row.row.legacy_operation_id.bytes[0] != bucket.shard)
					return status::invalid;
				if (!reference_current_census(
					    work, &bucket, &row,
					    FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES +
						    reference_current_encode_frames,
					    reserve, context))
					return status::capacity;
				row.result = flatfile_item_accounting_reference_encode(
					row.row, &row.canonical);
				if (row.result != status::ok)
					return row.result;
				if (row.canonical.size() != row.bytes.size() ||
				    !std::equal(row.canonical.begin(), row.canonical.end(),
						row.bytes.begin()))
					return status::invalid;
				// Each original unique index is global across all shards. Admit a
				// real GCC13 tree node and the complete temporary key/result pair
				// before each original emplace, then census actual retained sizes.
				if (!reference_current_census(
					    work, &bucket, &row,
					    sizeof(reference_current_key_node) +
						    sizeof(reference_current_key) +
						    sizeof(std::pair<
							    std::set<reference_current_key>::iterator,
							    bool>),
					    reserve, context))
					return status::capacity;
				if (!work.operation_lines
					     .emplace(row.row.operation_id.bytes,
						      row.row.line_index)
					     .second)
					return status::invalid;
				if (!reference_current_census(
					    work, &bucket, &row,
					    sizeof(reference_current_key_node) +
						    sizeof(reference_current_key) +
						    sizeof(std::pair<
							    std::set<reference_current_key>::iterator,
							    bool>),
					    reserve, context))
					return status::capacity;
				if (!work.legacy_events
					     .emplace(row.row.legacy_operation_id.bytes,
						      row.row.legacy_event_index)
					     .second)
					return status::invalid;
				row.root_match = row.row.operation_id.bytes == operation.bytes;
				row.legacy_match = row.row.legacy_operation_id.bytes ==
						   operation.bytes;
				if (row.root_match || row.legacy_match)
				{
					if (!row.root_match || !row.legacy_match)
						return status::invalid;
					if (!reference_current_census(
						    work, &bucket, &row,
						    sizeof(std::vector<
							    economic_accounting_item_reference>::
								   const_iterator) *
								    3 +
							    sizeof(uint16_t) +
							    sizeof(reference_current_line_less),
						    reserve, context))
						return status::capacity;
					row.found = std::lower_bound(work.sorted.cbegin(),
								     work.sorted.cend(),
								     row.row.line_index, work.less);
					if (row.found == work.sorted.cend() ||
					    row.found->line_index != row.row.line_index)
						return status::invalid;
					if (!reference_current_census(
						    work, &bucket, &row,
						    sizeof(reference_current_line_node) +
							    sizeof(uint16_t) +
							    sizeof(std::pair<
								    std::set<uint16_t>::iterator,
								    bool>),
						    reserve, context))
						return status::capacity;
					if (!work.matched.insert(row.row.line_index).second)
						return status::invalid;
					if (!reference_current_census(
						    work, &bucket, &row,
						    FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES +
							    reference_current_encode_frames,
						    reserve, context))
						return status::capacity;
					row.result = flatfile_item_accounting_reference_encode(
						*row.found, &row.expected_bytes);
					if (row.result != status::ok)
						return row.result;
					if (row.canonical != row.expected_bytes)
						return status::invalid;
					++work.observed.root_records;
					++work.observed.legacy_records;
				}
				++work.observed.retained_records;
			}
			++work.observed.buckets_verified;
		}
		if (work.observed.buckets_verified != 256 || !lock.matches(root))
			return status::invalid;
		if (work.observed.root_records != expected.size() ||
		    work.observed.legacy_records != expected.size() ||
		    work.matched.size() != expected.size())
			return work.selected_missing && !expected.empty() ? status::not_found :
									    status::invalid;
		// Every allocating/fallible reserve is before this strong scalar transfer.
		*output = work.observed;
		return status::ok;
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
