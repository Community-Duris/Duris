#include "flatfile/flatfile_item_accounting_reference.h"
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
