#include "flatfile/flatfile_item_accounting_reference.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
	std::string temp_dir = "/tmp/test_flatfile_item_refs";
	if (argc > 1)
		temp_dir = argv[1];

	fs::remove_all(temp_dir);
	fs::create_directories(temp_dir + "/accounting/item_references");
	fs::permissions(temp_dir, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(temp_dir + "/accounting", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(temp_dir + "/accounting/item_references", fs::perms::owner_all,
			fs::perm_options::replace);

	economic_accounting_item_reference ref1 = {};
	ref1.operation_id.bytes[0] = 0x11;
	ref1.line_index = 5;
	ref1.event_index = 5;
	ref1.child_index = 1;
	ref1.item_uid = 5001;
	ref1.before_revision = 10;
	ref1.after_revision = 11;
	ref1.legacy_operation_id.bytes[0] = 0xaa;
	ref1.legacy_operation_id.bytes[15] = 0xbb;
	ref1.legacy_event_index = 2;

	// 1. Encode
	std::vector<uint8_t> encoded;
	auto enc_status = flatfile_item_accounting_reference_encode(ref1, &encoded);
	assert(enc_status == flatfile_item_accounting_status::ok);
	assert(encoded.size() == FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);

	// 2. Decode
	economic_accounting_item_reference decoded = {};
	auto dec_status = flatfile_item_accounting_reference_decode(encoded, &decoded);
	assert(dec_status == flatfile_item_accounting_status::ok);
	assert(decoded.operation_id.bytes == ref1.operation_id.bytes);
	assert(decoded.line_index == 5);
	assert(decoded.event_index == 5);
	assert(decoded.child_index == 1);
	assert(decoded.item_uid == 5001);
	assert(decoded.before_revision == 10);
	assert(decoded.after_revision == 11);
	assert(decoded.legacy_operation_id.bytes == ref1.legacy_operation_id.bytes);
	assert(decoded.legacy_event_index == 2);
	auto overlong = encoded;
	overlong.push_back(0);
	decoded = {};
	decoded.item_uid = 999;
	assert(flatfile_item_accounting_reference_decode(overlong, &decoded) ==
	       flatfile_item_accounting_status::invalid);
	assert(decoded.item_uid == 999);

	// 3. Append to flatfile store
	std::string err;
	auto app_status = flatfile_item_accounting_reference_append(temp_dir, ref1, &err);
	assert(app_status == flatfile_item_accounting_status::ok);

	// Append a second reference
	economic_accounting_item_reference ref2 = ref1;
	ref2.line_index = 6;
	ref2.event_index = 6;
	ref2.item_uid = 5002;
	ref2.before_revision = 20;
	ref2.after_revision = 21;
	ref2.legacy_event_index = 3;
	app_status = flatfile_item_accounting_reference_append(temp_dir, ref2, &err);
	assert(app_status == flatfile_item_accounting_status::ok);

	// 4. Find by legacy
	economic_accounting_item_reference found = {};
	auto find_status = flatfile_item_accounting_reference_find_by_legacy(
		temp_dir, ref1.legacy_operation_id, 2, &found, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(found.item_uid == 5001);
	assert(found.after_revision == 11);

	find_status = flatfile_item_accounting_reference_find_by_legacy(
		temp_dir, ref1.legacy_operation_id, 3, &found, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(found.item_uid == 5002);

	// Missing legacy event index
	find_status = flatfile_item_accounting_reference_find_by_legacy(
		temp_dir, ref1.legacy_operation_id, 99, &found, &err);
	assert(find_status == flatfile_item_accounting_status::not_found);

	// 5. Find by item UID and revision
	find_status =
		flatfile_item_accounting_reference_find_by_item(temp_dir, 5001, 11, &found, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(found.line_index == 5);

	find_status =
		flatfile_item_accounting_reference_find_by_item(temp_dir, 5002, 21, &found, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(found.line_index == 6);

	find_status =
		flatfile_item_accounting_reference_find_by_item(temp_dir, 9999, 1, &found, &err);
	assert(find_status == flatfile_item_accounting_status::not_found);

	// 6. Append a 3rd reference to verify history for item 5001
	economic_accounting_item_reference ref3 = ref1;
	ref3.line_index = 7;
	ref3.event_index = 7;
	ref3.item_uid = 5001;
	ref3.before_revision = 11;
	ref3.after_revision = 12;
	ref3.legacy_event_index = 4;
	app_status = flatfile_item_accounting_reference_append(temp_dir, ref3, &err);
	assert(app_status == flatfile_item_accounting_status::ok);

	// 7. Find by operation
	std::vector<economic_accounting_item_reference> op_refs;
	find_status = flatfile_item_accounting_reference_find_by_operation(
		temp_dir, ref1.operation_id, &op_refs, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(op_refs.size() == 3);
	assert(op_refs[0].line_index == 5);
	assert(op_refs[1].line_index == 6);
	assert(op_refs[2].line_index == 7);

	critical_operation_id missing_op = {};
	missing_op.bytes[0] = 0x99;
	find_status = flatfile_item_accounting_reference_find_by_operation(temp_dir, missing_op,
									   &op_refs, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(op_refs.empty());

	// 8. Find history by item UID
	std::vector<economic_accounting_item_reference> history;
	find_status =
		flatfile_item_accounting_reference_find_history(temp_dir, 5001, &history, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(history.size() == 2);
	assert(history[0].after_revision == 11);
	assert(history[1].after_revision == 12);

	find_status =
		flatfile_item_accounting_reference_find_history(temp_dir, 9999, &history, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(history.empty());

	// 9. Verify __NO_MYSQL__ stubs return false
	assert(!economic_accounting_item_reference_find_by_operation(nullptr, ref1.operation_id,
								     &op_refs));
	assert(!economic_accounting_item_reference_find_history(nullptr, 5001, &history));

	// 10. A truncated final record invalidates the whole bucket. Readers must not
	// report a clean lookup, and append must not extend or replace the evidence.
	const auto bucket = fs::path(temp_dir) / "accounting" / "item_references" / "aa.bin";
	{
		std::ofstream duplicate(bucket, std::ios::binary | std::ios::app);
		assert(duplicate.write(reinterpret_cast<const char *>(encoded.data()),
				       static_cast<std::streamsize>(encoded.size())));
	}
	const auto duplicate_size = fs::file_size(bucket);
	found = ref2;
	find_status = flatfile_item_accounting_reference_find_by_legacy(
		temp_dir, ref1.legacy_operation_id, 2, &found, &err);
	assert(find_status == flatfile_item_accounting_status::invalid);
	assert(found.item_uid == ref2.item_uid);
	find_status = flatfile_item_accounting_reference_find_by_item(
		temp_dir, ref1.item_uid, ref1.after_revision, &found, &err);
	assert(find_status == flatfile_item_accounting_status::invalid);
	assert(found.item_uid == ref2.item_uid);
	assert(flatfile_item_accounting_reference_append(temp_dir, ref1, &err) ==
	       flatfile_item_accounting_status::already_exists);
	assert(fs::file_size(bucket) == duplicate_size);
	{
		std::ofstream partial(bucket, std::ios::binary | std::ios::app);
		assert(partial && partial.put('\x7f'));
	}
	const auto partial_size = fs::file_size(bucket);
	found = ref1;
	find_status = flatfile_item_accounting_reference_find_by_legacy(
		temp_dir, ref1.legacy_operation_id, 2, &found, &err);
	assert(find_status == flatfile_item_accounting_status::invalid);
	assert(found.item_uid == ref1.item_uid);
	find_status = flatfile_item_accounting_reference_find_by_item(
		temp_dir, ref1.item_uid, ref1.after_revision, &found, &err);
	assert(find_status == flatfile_item_accounting_status::invalid);
	assert(found.item_uid == ref1.item_uid);
	assert(flatfile_item_accounting_reference_append(temp_dir, ref2, &err) ==
	       flatfile_item_accounting_status::invalid);
	assert(fs::file_size(bucket) == partial_size);

	// A full-size but invalid record also invalidates the bucket instead of
	// being skipped while a later matching record is returned.
	fs::resize_file(bucket, partial_size - 1);
	{
		std::fstream corrupted(bucket, std::ios::binary | std::ios::in | std::ios::out);
		assert(corrupted);
		corrupted.put('\0');
		assert(corrupted);
	}
	const auto corrupted_size = fs::file_size(bucket);
	found = ref1;
	find_status = flatfile_item_accounting_reference_find_by_legacy(
		temp_dir, ref1.legacy_operation_id, 2, &found, &err);
	assert(find_status == flatfile_item_accounting_status::invalid);
	assert(found.item_uid == ref1.item_uid);
	find_status = flatfile_item_accounting_reference_find_by_item(
		temp_dir, ref1.item_uid, ref1.after_revision, &found, &err);
	assert(find_status == flatfile_item_accounting_status::invalid);
	assert(found.item_uid == ref1.item_uid);
	assert(flatfile_item_accounting_reference_append(temp_dir, ref2, &err) ==
	       flatfile_item_accounting_status::invalid);
	assert(fs::file_size(bucket) == corrupted_size);

	// Appending at the read limit must fail before making a bucket that lookups
	// cannot load again.
	const size_t capacity_size = (FLATFILE_ITEM_ACCOUNTING_REFERENCE_BUCKET_MAX_BYTES /
				      FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES) *
				     FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES;
	fs::resize_file(bucket, capacity_size);
	assert(flatfile_item_accounting_reference_append(temp_dir, ref2, &err) ==
	       flatfile_item_accounting_status::capacity);
	assert(fs::file_size(bucket) == capacity_size);

	// Test verify_and_repair on a bucket with trailing partial bytes from a simulated crash
	fs::remove(bucket);
	assert(flatfile_item_accounting_reference_append(temp_dir, ref1, &err) ==
	       flatfile_item_accounting_status::ok);
	assert(fs::file_size(bucket) == FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
	// Append 17 partial trailing garbage bytes
	{
		std::ofstream crash_append(bucket, std::ios::binary | std::ios::app);
		std::string partial_garbage = "partial crash bytes";
		crash_append.write(partial_garbage.data(), 17);
	}
	assert(fs::file_size(bucket) == FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES + 17);

	flatfile_accounting_audit_report report = {};
	assert(flatfile_item_accounting_reference_verify_and_repair(temp_dir, &report, &err) ==
	       flatfile_item_accounting_status::ok);
	assert(report.total_buckets_scanned == 1);
	assert(report.valid_records_count == 1);
	assert(report.partial_trailing_bytes_truncated == 17);
	assert(fs::file_size(bucket) == FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);

	// After repair, subsequent lookups and appends succeed without error
	found = {};
	assert(flatfile_item_accounting_reference_find_by_item(temp_dir, ref1.item_uid,
							       ref1.after_revision, &found, &err) ==
	       flatfile_item_accounting_status::ok);
	assert(found.item_uid == ref1.item_uid);

	// Two child operations can share a shard in one authority transaction.
	// Staging also creates the private reference directory for a fresh root.
	const std::string staged_root = temp_dir + "/staged";
	fs::create_directories(staged_root + "/domains");
	fs::permissions(staged_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(staged_root + "/domains", fs::perms::owner_all, fs::perm_options::replace);
	flatfile_authority_lock lock;
	assert(lock.acquire(staged_root, &err));
	economic_accounting_item_reference staged_first = ref1;
	staged_first.legacy_operation_id.bytes[0] = 0xbb;
	staged_first.legacy_operation_id.bytes[15] = 1;
	staged_first.legacy_event_index = 0;
	economic_accounting_item_reference staged_second = ref2;
	staged_second.legacy_operation_id.bytes[0] = 0xbb;
	staged_second.legacy_operation_id.bytes[15] = 2;
	staged_second.legacy_event_index = 0;
	std::vector<flatfile_authority_operation> operations;
	assert(flatfile_item_accounting_reference_stage(
		       staged_root, lock, staged_first.legacy_operation_id,
		       std::span<const economic_accounting_item_reference>(&staged_first, 1),
		       &operations, &err) == flatfile_item_accounting_status::ok);
	assert(flatfile_item_accounting_reference_stage(
		       staged_root, lock, staged_second.legacy_operation_id,
		       std::span<const economic_accounting_item_reference>(&staged_second, 1),
		       &operations, &err) == flatfile_item_accounting_status::ok);
	assert(operations.size() == 1 && operations[0].filename == "bb.bin" &&
	       operations[0].bytes.size() == 2 * FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES);
	assert(flatfile_authority_transaction_commit_operations(staged_root, lock, operations,
								&err) ==
	       flatfile_authority_transaction_result::ok);
	assert(flatfile_item_accounting_reference_verify_operation(
		       staged_root, staged_first.legacy_operation_id,
		       std::span<const economic_accounting_item_reference>(&staged_first, 1),
		       &err) == flatfile_item_accounting_status::ok);
	assert(flatfile_item_accounting_reference_verify_operation(
		       staged_root, staged_second.legacy_operation_id,
		       std::span<const economic_accounting_item_reference>(&staged_second, 1),
		       &err) == flatfile_item_accounting_status::ok);

	fs::remove_all(temp_dir);
	return 0;
}
