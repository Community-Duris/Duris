#include "flatfile/flatfile_item_accounting_reference.h"
#include "economy/collector_command.h"

#include <cassert>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
	assert(argc > 1);
	std::string root = argv[1];

	// Simulate collector buyback item transfer reference recording
	economic_accounting_item_reference buyback_ref = {};
	buyback_ref.operation_id.bytes[0] = 0x88;
	buyback_ref.line_index = 0;
	buyback_ref.event_index = 0;
	buyback_ref.child_index = 1;
	buyback_ref.item_uid = 998877;
	buyback_ref.before_revision = 5;
	buyback_ref.after_revision = 6;
	buyback_ref.legacy_operation_id.bytes[0] = 0x88;
	buyback_ref.legacy_event_index = 0;

	// Append reference
	auto append_status = flatfile_item_accounting_reference_append(root, buyback_ref);
	assert(append_status == flatfile_item_accounting_status::ok);

	// Find by legacy operation id and event index
	economic_accounting_item_reference found_legacy = {};
	auto find_legacy_status = flatfile_item_accounting_reference_find_by_legacy(
		root, buyback_ref.legacy_operation_id, buyback_ref.legacy_event_index,
		&found_legacy);
	assert(find_legacy_status == flatfile_item_accounting_status::ok);
	assert(found_legacy.item_uid == 998877);
	assert(found_legacy.before_revision == 5);
	assert(found_legacy.after_revision == 6);

	// Find by item uid and revision
	economic_accounting_item_reference found_item = {};
	auto find_item_status =
		flatfile_item_accounting_reference_find_by_item(root, 998877, 6, &found_item);
	assert(find_item_status == flatfile_item_accounting_status::ok);
	assert(found_item.operation_id.bytes[0] == 0x88);

	// Simulate multiple items collected from corpse by collector
	for (uint16_t idx = 0; idx < 3; ++idx)
	{
		economic_accounting_item_reference collect_ref = {};
		collect_ref.operation_id.bytes[0] = 0x99;
		collect_ref.line_index = idx;
		collect_ref.event_index = idx;
		collect_ref.child_index = 1;
		collect_ref.item_uid = 1000 + idx;
		collect_ref.before_revision = 1;
		collect_ref.after_revision = 2;
		collect_ref.legacy_operation_id.bytes[0] = 0x99;
		collect_ref.legacy_event_index = idx;

		append_status = flatfile_item_accounting_reference_append(root, collect_ref);
		assert(append_status == flatfile_item_accounting_status::ok);

		economic_accounting_item_reference item_check = {};
		find_item_status = flatfile_item_accounting_reference_find_by_item(root, 1000 + idx,
										   2, &item_check);
		assert(find_item_status == flatfile_item_accounting_status::ok);
		assert(item_check.line_index == idx);
		assert(item_check.event_index == idx);
	}

	return 0;
}
