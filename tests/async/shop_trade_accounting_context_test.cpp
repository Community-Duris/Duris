#include "flatfile/flatfile_item_accounting_reference.h"
#include "economy/shop_trade_command.h"

#include <cassert>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
	assert(argc > 1);
	std::string root = argv[1];

	// Test 1: buy_existing accounting reference
	economic_accounting_item_reference buy_ref = {};
	buy_ref.operation_id.bytes[0] = 0xAA;
	buy_ref.line_index = 0;
	buy_ref.event_index = 0;
	buy_ref.child_index = 1;
	buy_ref.item_uid = 555111;
	buy_ref.before_revision = 3;
	buy_ref.after_revision = 4;
	buy_ref.legacy_operation_id.bytes[0] = 0xAA;
	buy_ref.legacy_event_index = 0;

	auto append_status = flatfile_item_accounting_reference_append(root, buy_ref);
	assert(append_status == flatfile_item_accounting_status::ok);

	economic_accounting_item_reference found_buy = {};
	auto find_status = flatfile_item_accounting_reference_find_by_legacy(
		root, buy_ref.legacy_operation_id, buy_ref.legacy_event_index, &found_buy);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(found_buy.item_uid == 555111);
	assert(found_buy.before_revision == 3);
	assert(found_buy.after_revision == 4);

	// Test 2: buy_produced accounting reference
	economic_accounting_item_reference produced_ref = {};
	produced_ref.operation_id.bytes[0] = 0xBB;
	produced_ref.line_index = 0;
	produced_ref.event_index = 0;
	produced_ref.child_index = 1;
	produced_ref.item_uid = 555222;
	produced_ref.before_revision = 0;
	produced_ref.after_revision = 1;
	produced_ref.legacy_operation_id.bytes[0] = 0xBB;
	produced_ref.legacy_event_index = 0;

	append_status = flatfile_item_accounting_reference_append(root, produced_ref);
	assert(append_status == flatfile_item_accounting_status::ok);

	economic_accounting_item_reference found_produced = {};
	find_status =
		flatfile_item_accounting_reference_find_by_item(root, 555222, 1, &found_produced);
	assert(find_status == flatfile_item_accounting_status::ok);
	assert(found_produced.operation_id.bytes[0] == 0xBB);

	// Test 3: sell_store multiple items accounting reference
	for (uint16_t idx = 0; idx < 4; ++idx)
	{
		economic_accounting_item_reference sell_ref = {};
		sell_ref.operation_id.bytes[0] = 0xCC;
		sell_ref.line_index = idx;
		sell_ref.event_index = idx;
		sell_ref.child_index = 1;
		sell_ref.item_uid = 777000 + idx;
		sell_ref.before_revision = 10;
		sell_ref.after_revision = 11;
		sell_ref.legacy_operation_id.bytes[0] = 0xCC;
		sell_ref.legacy_event_index = idx;

		append_status = flatfile_item_accounting_reference_append(root, sell_ref);
		assert(append_status == flatfile_item_accounting_status::ok);

		economic_accounting_item_reference found_sell = {};
		find_status = flatfile_item_accounting_reference_find_by_item(root, 777000 + idx,
									      11, &found_sell);
		assert(find_status == flatfile_item_accounting_status::ok);
		assert(found_sell.line_index == idx);
		assert(found_sell.event_index == idx);
	}

	return 0;
}
