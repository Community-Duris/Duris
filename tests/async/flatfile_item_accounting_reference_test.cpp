#include "flatfile/flatfile_item_accounting_reference.h"
#include <cassert>
#include <filesystem>
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

	fs::remove_all(temp_dir);
	return 0;
}
