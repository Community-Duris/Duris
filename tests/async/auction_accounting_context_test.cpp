#include "flatfile/flatfile_item_accounting_reference.h"
#include "economy/auction_command.h"

#include <cassert>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
	assert(argc > 1);
	std::string root = argv[1];

	// Test 1: auction list accounting reference (player -> auction)
	economic_accounting_item_reference list_ref = {};
	list_ref.operation_id.bytes[0] = 0xDE;
	list_ref.line_index = 0;
	list_ref.event_index = 0;
	list_ref.child_index = 1;
	list_ref.item_uid = 600100;
	list_ref.before_revision = 7;
	list_ref.after_revision = 8;
	list_ref.legacy_operation_id.bytes[0] = 0xDE;
	list_ref.legacy_event_index = 0;

	auto append_status = flatfile_item_accounting_reference_append(root, list_ref);
	assert(append_status == flatfile_item_accounting_status::ok);

	economic_accounting_item_reference found_list = {};
	auto find_status = flatfile_item_accounting_reference_find_by_legacy(
		root, list_ref.legacy_operation_id, list_ref.legacy_event_index, &found_list);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(found_list.item_uid == 600100);
	assert(found_list.before_revision == 7);
	assert(found_list.after_revision == 8);

	// Test 2: auction claim accounting reference (auction -> winner)
	economic_accounting_item_reference claim_ref = {};
	claim_ref.operation_id.bytes[0] = 0xDF;
	claim_ref.line_index = 0;
	claim_ref.event_index = 0;
	claim_ref.child_index = 1;
	claim_ref.item_uid = 600100;
	claim_ref.before_revision = 8;
	claim_ref.after_revision = 9;
	claim_ref.legacy_operation_id.bytes[0] = 0xDF;
	claim_ref.legacy_event_index = 0;

	append_status = flatfile_item_accounting_reference_append(root, claim_ref);
	assert(append_status == flatfile_item_accounting_status::ok);

	economic_accounting_item_reference found_claim = {};
	find_status =
		flatfile_item_accounting_reference_find_by_item(root, 600100, 9, &found_claim);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(found_claim.operation_id.bytes[0] == 0xDF);
	assert(found_claim.after_revision == 9);

	return 0;
}
