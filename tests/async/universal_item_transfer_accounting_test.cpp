#include "flatfile/flatfile_item_accounting_reference.h"
#include "item/item_transfer_command.h"

#include <cassert>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
	assert(argc > 1);
	std::string root = argv[1];

	// Create test accounting reference
	economic_accounting_item_reference ref = {};
	ref.operation_id.bytes[0] = 0x77;
	ref.line_index = 0;
	ref.event_index = 0;
	ref.child_index = 1;
	ref.item_uid = 424242;
	ref.before_revision = 1;
	ref.after_revision = 2;
	ref.legacy_operation_id.bytes[0] = 0x77;
	ref.legacy_event_index = 0;

	// Append reference
	auto append_status = flatfile_item_accounting_reference_append(root, ref);
	assert(append_status == flatfile_item_accounting_status::ok);

	// Find by legacy operation id
	economic_accounting_item_reference found_legacy = {};
	auto find_legacy_status = flatfile_item_accounting_reference_find_by_legacy(
		root, ref.legacy_operation_id, ref.legacy_event_index, &found_legacy);
	assert(find_legacy_status == flatfile_item_accounting_status::ok);
	assert(found_legacy.item_uid == 424242);
	assert(found_legacy.after_revision == 2);

	// Find by item uid and revision
	economic_accounting_item_reference found_item = {};
	auto find_item_status =
		flatfile_item_accounting_reference_find_by_item(root, 424242, 2, &found_item);
	assert(find_item_status == flatfile_item_accounting_status::ok);
	assert(found_item.operation_id.bytes[0] == 0x77);

	return 0;
}
