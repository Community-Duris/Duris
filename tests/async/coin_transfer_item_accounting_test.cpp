#include "economy/coin_transfer_command.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "item/economic_accounting_item_reference.h"

#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
	assert(argc > 1);
	std::string root = argv[1];

	critical_operation_id root_op = {};
	root_op.bytes[0] = 0xc0;
	root_op.bytes[1] = 0x14;

	critical_operation_id child_item_op = {};
	child_item_op.bytes[0] = 0xde;
	child_item_op.bytes[1] = 0x57;

	// In a coin transfer, endpoint 0 may be wallet and endpoint 1 is a physical coin pile.
	// The item reference links the physical item custody change to the root coin transfer operation.
	economic_accounting_item_reference ref = {};
	ref.operation_id = root_op;
	ref.line_index = 0;
	ref.event_index = 0;
	ref.child_index = 2; // endpoint index 1 -> child_index 2
	ref.item_uid = 998877;
	ref.before_revision = 0;
	ref.after_revision = 1;
	ref.legacy_operation_id = child_item_op;
	ref.legacy_event_index = 0;

	// 1. Invariant validation
	assert(economic_accounting_item_reference_validate(ref));

	// 2. Append reference to flatfile store
	std::string err;
	auto append_status = flatfile_item_accounting_reference_append(root, ref, &err);
	assert(append_status == flatfile_item_accounting_status::ok);

	// 3. Find by root operation (Slice 11 provenance query)
	std::vector<economic_accounting_item_reference> op_refs;
	auto find_status =
		flatfile_item_accounting_reference_find_by_operation(root, root_op, &op_refs, &err);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(op_refs.size() == 1);
	assert(op_refs[0].child_index == 2);
	assert(op_refs[0].item_uid == 998877);
	assert(op_refs[0].after_revision == 1);
	assert(op_refs[0].legacy_operation_id.bytes == child_item_op.bytes);

	// 4. Find history by item UID
	std::vector<economic_accounting_item_reference> history;
	auto history_status =
		flatfile_item_accounting_reference_find_history(root, 998877, &history, &err);
	assert(history_status == flatfile_item_accounting_status::ok);
	assert(history.size() == 1);
	assert(history[0].operation_id.bytes == root_op.bytes);

	return 0;
}
