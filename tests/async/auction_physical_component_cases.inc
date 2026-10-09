// Additive controls in the original auction transaction fixture. Framed DTOs
// below are manufactured component inputs, never SQL provenance or authority.
#include "economy/auction_repository.h"
// Original shared framing helper is included once by the original room driver.
#include <algorithm>
#include <array>
#include <new>

namespace
{
bool auction_fault_enabled = false;
size_t auction_fault_target = 0, auction_fault_seen = 0;
}
void *operator new(size_t size)
{
	if (auction_fault_enabled && ++auction_fault_seen == auction_fault_target)
		throw std::bad_alloc();
	if (void *value = std::malloc(size ? size : 1))
		return value;
	throw std::bad_alloc();
}
void *operator new[](size_t size)
{
	return ::operator new(size);
}
void operator delete(void *value) noexcept
{
	std::free(value);
}
void operator delete[](void *value) noexcept
{
	std::free(value);
}
void operator delete(void *value, size_t) noexcept
{
	std::free(value);
}
void operator delete[](void *value, size_t) noexcept
{
	std::free(value);
}

namespace
{
const std::string auction_test_op(16, 'a');
struct auction_component_packet
{
	economic_sql_physical_source_snapshot base;
	sql_room_item_source_snapshot supplement;
};
template <class T> void auction_test_integer(std::string &value, T number)
{
	for (size_t i = 0; i < sizeof(T); ++i)
		value.push_back(static_cast<char>(uint64_t(number) >> (8 * i)));
}
player_item_snapshot auction_test_item(uint64_t uid, int32_t parent = -1)
{
	player_item_snapshot item{};
	item.object_uid = uid;
	item.parent_index = parent;
	item.vnum = 77;
	item.string_mask = 15;
	item.name = "literal";
	item.short_description = "literal";
	item.description = "literal";
	item.action_description = "literal";
	return item;
}
std::string auction_test_blob(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> encoded;
	assert(player_item_snapshot_list_encode(items, &encoded) ==
	       player_snapshot_codec_result::ok);
	std::string blob;
	auction_test_integer(blob, uint32_t{ 0x32544341 });
	auction_test_integer(blob, static_cast<uint32_t>(items.size()));
	auction_test_integer(blob, static_cast<uint32_t>(encoded.size()));
	for (size_t n = 0; n < items.size(); ++n)
	{
		auction_test_integer(blob, items[n].object_uid);
		auction_test_integer(blob, uint64_t{ 2 + n });
	}
	blob.append(reinterpret_cast<const char *>(encoded.data()), encoded.size());
	return blob;
}
void auction_test_table_frames(std::vector<economic_sql_source_table> &tables)
{
	for (auto &table : tables)
	{
		equipment_test_digest content;
		content.text("EST1");
		content.bytes(table.definition_digest);
		content.number(table.rows.size());
		for (auto &row : table.rows)
		{
			equipment_test_digest hash;
			hash.text("ESR1");
			hash.bytes(table.definition_digest);
			for (const auto &cell : row.cells)
			{
				hash.number(cell ? 1 : 0);
				if (cell)
					hash.text(*cell);
			}
			row.digest = hash.finish();
			content.bytes(row.digest);
		}
		table.content_digest = content.finish();
	}
}
void auction_test_reframe(auction_component_packet &packet)
{
	reframe_physical_component(packet.base);
	auction_test_table_frames(packet.supplement.tables);
	packet.supplement.physical_digest = packet.base.digest;
	packet.supplement.rows = packet.base.rows;
	packet.supplement.cells = packet.base.cells;
	packet.supplement.cell_bytes = packet.base.cell_bytes;
	for (const auto &table : packet.supplement.tables)
		for (const auto &row : table.rows)
		{
			++packet.supplement.rows;
			packet.supplement.cells += row.cells.size();
			for (const auto &cell : row.cells)
				if (cell)
					packet.supplement.cell_bytes += cell->size();
		}
}
auction_component_packet auction_test_packet(size_t roots = 1, size_t nodes_per_root = 2)
{
	auction_component_packet packet;
	auto &value = packet.base;
#include "component_registry.inc"
	for (const auto &spec : std::vector<std::array<const char *, 3>>{
		     { "sql_room_item_payload",
		       "item_uid,item_revision,payload_version,operation_id,season_epoch,payload",
		       "item_uid,item_revision" },
		     { "season_reset_state", "state_id,season_epoch,reset_status", "state_id" },
		     { "item_ownership_ledger",
		       "operation_id,event_index,item_uid,root_item_uid,parent_item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site,from_equipment_slot,to_equipment_slot",
		       "operation_id,event_index" },
		     { "economic_accounting_item_reference",
		       "operation_id,line_index,event_index,child_index,item_uid,before_revision,after_revision,legacy_operation_id,legacy_event_index",
		       "operation_id,line_index" },
		     { "economic_accounting_operation", "operation_id,outcome,result_code",
		       "operation_id" } })
		packet.supplement.tables.push_back(component_table(spec[0], spec[1], spec[2]));
	auction_command_result receipt{};
	receipt.action = auction_action::list;
	receipt.event_type = auction_event_type::listed;
	receipt.auction_id = 10;
	receipt.seller_pid = 7;
	receipt.status = 1;
	receipt.auction_revision = 1;
	receipt.player_owner_revision = 1;
	receipt.auction_owner_revision = 1;
	receipt.item_count = static_cast<uint16_t>(roots);
	for (size_t slot = 0; slot < roots; ++slot)
	{
		receipt.item_uids[slot] = 100 + slot * 10000;
		receipt.item_revisions[slot] = 2;
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded{};
	assert(auction_command_encode_result(receipt, &encoded));
	const std::string payload(reinterpret_cast<const char *>(encoded.data()), encoded.size());
	component_row(value.source2.tables[4],
		      { "10", "7", "OPEN", "0", "0", "0", std::to_string(roots), "1", "1",
			auction_test_op, "77", "v1 original blob" });
	component_row(value.source2.tables[17],
		      { auction_test_op, std::string(32, 'h'), std::string(32, 'k'), "7", "2", "2",
			"1", "0", "0", "1", payload, "1" });
	component_row(value.source2.tables[18], { "1", auction_test_op, "0", "5", "1", "1", payload,
						  "0", "0", "0", "0", "0" });
	component_row(value.source2.tables[12], { "6", "10", "0", "1" });
	component_row(packet.supplement.tables[4], { auction_test_op, "1", "0" });
	size_t event = 0;
	for (size_t slot = 0; slot < roots; ++slot)
	{
		const uint64_t root = receipt.item_uids[slot];
		std::vector<player_item_snapshot> items;
		for (size_t n = 0; n < nodes_per_root; ++n)
			items.push_back(auction_test_item(root + n, n ? 0 : -1));
		component_row(value.source2.tables[7],
			      { "10", std::to_string(slot), std::to_string(root), "2", "77",
				auction_test_blob(items), std::nullopt, std::nullopt, "0" });
		for (size_t n = 0; n < items.size(); ++n, ++event)
		{
			const auto uid = std::to_string(root + n), revision = std::to_string(2 + n),
				   ev = std::to_string(event);
			const std::optional<std::string> parent =
				n ? std::optional<std::string>(std::to_string(root)) : std::nullopt;
			component_row(value.source2.tables[11],
				      { uid, std::to_string(root), parent, "6", "10", "0", revision,
					"77", "1", std::nullopt });
			component_row(value.source2.item_equipment_sources[0], { uid, "0" });
			component_row(packet.supplement.tables[2],
				      { auction_test_op, ev, uid, std::to_string(root), parent, "1",
					"7", "0", "6", "10", "0", revision, "1", "1", "14", "10",
					"1", "0", "0" });
			component_row(packet.supplement.tables[3],
				      { auction_test_op, ev, ev, "0", uid, std::to_string(1 + n),
					revision, auction_test_op, ev });
		}
	}
	auction_test_reframe(packet);
	return packet;
}
auction_physical_correspondence auction_test_inspect(auction_component_packet &packet,
						     size_t limit = 512)
{
	auction_test_reframe(packet);
	auction_physical_correspondence report;
	assert(!auction_repository_inspect_physical_sources(packet.base, packet.supplement, {},
							    limit, &report));
	assert(report.physical_digest == packet.base.digest);
	return report;
}
uint64_t auction_test_findings(const auction_physical_correspondence &report,
			       auction_physical_issue issue)
{
	return report.issue_counts[static_cast<size_t>(issue)];
}
void auction_physical_component_cases()
{
	auto packet = auction_test_packet();
	auto original = auction_test_inspect(packet);
	assert(original.listings.size() == 1 && original.slots.size() == 1 &&
	       original.nodes.size() == 2);
	assert(original.listings[0].family == auction_physical_family::observed_native_v2);
	assert(original.nodes[0].current_field_correspondence &&
	       original.nodes[1].current_field_correspondence);
	assert(original.nodes[0].generic_proof == auction_physical_generic_proof::observed_match);
	assert(original.nodes[0].original_provenance_unknown &&
	       original.nodes[1].original_provenance_unknown);
	assert(original.missing_literal_events.empty() &&
	       original.unmatched_auction_custody.empty());
	for (bool old_operation : { false, true })
	{
		auto unbound = packet;
		unbound.base.source2.tables[4].rows[0].cells[9] = std::nullopt;
		if (old_operation)
		{
			auto old = unbound.base.source2.tables[17].rows[0];
			old.cells[0] = std::string(16, 'v');
			old.cells[4] = "1";
			old.cells[5] = "1";
			unbound.base.source2.tables[17].rows.push_back(old);
			unbound.base.source2.tables[4].rows[0].cells[9] = old.cells[0];
		}
		auto historical = auction_test_inspect(unbound);
		assert(historical.nodes.size() == 2 && historical.missing_literal_events.empty());
		for (const auto &node : historical.nodes)
			assert(!node.current_field_correspondence);
		assert(auction_test_findings(historical, auction_physical_issue::unbound_listing));
	}
	// Only strings change: row hash/literal changes, field correspondence survives,
	// original literal provenance remains UNKNOWN rather than falsely contradicted.
	auto changed = packet;
	auto items = std::vector<player_item_snapshot>{ auction_test_item(100),
							auction_test_item(101, 0) };
	items[1].description = "different canonical literal";
	changed.base.source2.tables[7].rows[0].cells[5] = auction_test_blob(items);
	auto literal = auction_test_inspect(changed);
	assert(literal.slots[0].source.digest != original.slots[0].source.digest);
	assert(literal.nodes[1].literal.description == items[1].description &&
	       literal.nodes[1].current_field_correspondence &&
	       literal.nodes[1].original_provenance_unknown);
	for (int mode = 0; mode < 3; ++mode)
	{
		auto missing = packet;
		if (mode == 0)
			missing.base.source2.tables[7].rows[0].cells[5] = std::nullopt;
		if (mode == 1)
			missing.base.source2.tables[7].rows[0].cells[5] = "ACT2-refused";
		if (mode == 2)
			missing.base.source2.tables[7].rows[0].cells[5]->pop_back();
		auto report = auction_test_inspect(missing);
		assert(report.nodes.empty() &&
		       auction_test_findings(report,
					     auction_physical_issue::literal_decode_refused));
		assert(report.missing_literal_events.size() == 2 &&
		       report.related_custody.size() == 2);
	}
	auto orphan = packet;
	orphan.base.source2.tables[4].rows.clear();
	auto surviving = auction_test_inspect(orphan);
	assert(surviving.listings.empty() && surviving.retained_listings.size() == 1 &&
	       !surviving.retained_listings[0].parent);
	assert(surviving.nodes.size() == 2 && surviving.missing_literal_events.empty());
	orphan.base.source2.tables[7].rows.clear();
	auto absent = auction_test_inspect(orphan);
	assert(absent.missing_literal_events.size() == 2 && absent.related_custody.size() == 2);
	// Entire foreign/malformed root membership survives capped diagnostic details.
	auto extra = packet;
	for (size_t n = 0; n < 520; ++n)
	{
		component_row(extra.base.source2.tables[11],
			      { std::to_string(1000 + n), "100", "100", n ? "1" : "bad-owner", "77",
				"0", "2", "77", "1", std::nullopt });
		component_row(extra.base.source2.item_equipment_sources[0],
			      { std::to_string(1000 + n), "0" });
	}
	auto extras = auction_test_inspect(extra, 1);
	assert(extras.related_custody.size() == 522 && extras.extra_custody.size() == 520);
	assert(extras.diagnostics.size() == 1 && extras.diagnostics_truncated &&
	       !extras.malformed_rows.empty());
	assert(!extras.nodes[0].current_field_correspondence);
	extra.base.source2.tables[7].rows.clear();
	auto refused_extra = auction_test_inspect(extra, 1);
	assert(refused_extra.related_custody.size() == 522);
	// A binary coin payload elsewhere is not a malformed auction scalar.
	auto coin = packet;
	coin.base.source2.tables[11].rows[1].cells[9] = std::string("\0\1blob", 6);
	assert(auction_test_inspect(coin).malformed_rows.empty());
	auto malformed = packet;
	malformed.base.source2.tables[11].rows[1].cells[0] = "not-an-integer";
	malformed.base.source2.item_equipment_sources[0].rows[1].cells[0] = "not-an-integer";
	auto bad_row = auction_test_inspect(malformed);
	assert(bad_row.related_custody.size() == 2 && !bad_row.malformed_rows.empty() &&
	       !bad_row.nodes[0].current_field_correspondence);
	auto wrong_root = packet;
	wrong_root.base.source2.tables[11].rows[0].cells[1] = "999";
	assert(auction_test_inspect(wrong_root).related_custody.size() == 2);
	auto staged = packet;
	staged.base.source2.tables[7].rows[0].cells[6] = "8";
	staged.base.source2.tables[7].rows[0].cells[7] = std::string(16, 'c');
	assert(auction_test_inspect(staged).nodes[0].current_field_correspondence);
	auto history = packet;
	auto old = history.base.source2.tables[7].rows[0];
	old.cells[0] = "9";
	old.cells[3] = "8";
	old.cells[6] = "8";
	old.cells[7] = std::string(16, 'c');
	old.cells[8] = "1";
	auto listing = history.base.source2.tables[4].rows[0];
	listing.cells[0] = "9";
	listing.cells[9] = std::string(16, 'b');
	history.base.source2.tables[4].rows.insert(history.base.source2.tables[4].rows.begin(),
						   listing);
	auto inbox = history.base.source2.tables[17].rows[0];
	inbox.cells[0] = std::string(16, 'b');
	auction_command_result old_receipt = original.retained_listings[0].receipt;
	old_receipt.auction_id = 9;
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> old_bytes{};
	assert(auction_command_encode_result(old_receipt, &old_bytes));
	inbox.cells[10] =
		std::string(reinterpret_cast<const char *>(old_bytes.data()), old_bytes.size());
	history.base.source2.tables[17].rows.push_back(inbox);
	history.base.source2.tables[7].rows.insert(history.base.source2.tables[7].rows.begin(),
						   old);
	auto retained = auction_test_inspect(history);
	assert(retained.slots.size() == 2 && retained.nodes.size() == 4 &&
	       *retained.slots[0].claimed);
	assert(!auction_test_findings(retained, auction_physical_issue::duplicate_current_uid));
	assert(!retained.nodes[0].current_field_correspondence &&
	       retained.nodes[2].current_field_correspondence);
	auto duplicated = packet;
	duplicated.base.source2.tables[11].rows.push_back(
		duplicated.base.source2.tables[11].rows[1]);
	duplicated.base.source2.item_equipment_sources[0].rows.push_back(
		duplicated.base.source2.item_equipment_sources[0].rows[1]);
	assert(!auction_test_inspect(duplicated).nodes[1].current_field_correspondence);
	for (int mode = 0; mode < 4; ++mode)
	{
		auto conflict = packet;
		if (mode == 0)
			conflict.base.source2.tables[7].rows[0].cells[1] = "9";
		if (mode == 1)
			conflict.base.source2.tables[17].rows[0].cells[9] = "9";
		if (mode == 2)
			conflict.base.source2.tables[17].rows[0].cells[10]->back() = 1;
		if (mode == 3)
			conflict.supplement.tables[2].rows[0].cells[16] = "65535";
		auto report = auction_test_inspect(conflict);
		assert(mode ? report.nodes.empty() ||
				       report.nodes[0].generic_proof !=
					       auction_physical_generic_proof::observed_match :
			      auction_test_findings(report, auction_physical_issue::invalid_slots));
	}
	auto legacy = packet;
	legacy.base.source2.tables[17].rows[0].cells[5] = "1";
	component_row(legacy.base.source2.tables[6],
		      { "1", "7", "ACT2-looking opaque legacy", "0", "1" });
	component_row(legacy.base.source2.tables[6], { "2", "7", "old opaque legacy", "1", "1" });
	auto opaque = auction_test_inspect(legacy);
	assert(opaque.nodes.empty() && opaque.pickups.size() == 2 &&
	       opaque.listings[0].family == auction_physical_family::legacy_or_v1);
	assert(*opaque.pickups[0].retrieved == 0 && *opaque.pickups[1].retrieved == 1);
	auto nine = auction_test_packet(9, 1);
	assert(auction_test_inspect(nine).nodes.size() == 9);
	auto ten = nine;
	ten.base.source2.tables[4].rows[0].cells[6] = "10";
	assert(auction_test_findings(auction_test_inspect(ten),
				     auction_physical_issue::invalid_slots));
	// Native combined node4096/4097 despite each root individually <=4096.
	auto forest = auction_test_packet(2, 2048);
	auto forest_report = auction_test_inspect(forest);
	assert(forest_report.nodes.size() == 4096 &&
	       !auction_test_findings(forest_report, auction_physical_issue::native_bound_refused));
	auto overflow = forest;
	items.clear();
	for (size_t n = 0; n < 2049; ++n)
		items.push_back(auction_test_item(10100 + n, n ? 0 : -1));
	overflow.base.source2.tables[7].rows[1].cells[5] = auction_test_blob(items);
	auto too_many = auction_test_inspect(overflow);
	assert(too_many.nodes.size() == 4097 &&
	       auction_test_findings(too_many, auction_physical_issue::native_bound_refused));
	for (const auto &node : too_many.nodes)
		assert(node.generic_proof != auction_physical_generic_proof::observed_match);
	// Combined property8192/8193 rows use the existing codec, no second grammar.
	auto property_rows = auction_test_packet(2, 1);
	for (size_t slot = 0; slot < 2; ++slot)
	{
		auto item = auction_test_item(100 + slot * 10000);
		item.dynamic_affects.resize(4095);
		property_rows.base.source2.tables[7].rows[slot].cells[5] =
			auction_test_blob({ item });
	}
	assert(!auction_test_findings(auction_test_inspect(property_rows),
				      auction_physical_issue::native_bound_refused));
	auto property = auction_test_item(10100);
	property.dynamic_affects.resize(4096);
	property_rows.base.source2.tables[7].rows[1].cells[5] = auction_test_blob({ property });
	assert(auction_test_findings(auction_test_inspect(property_rows),
				     auction_physical_issue::native_bound_refused));
	// Individually <=1MiB raw ACT2 cells can still exceed the combined4MiB
	// native codec bound; decoded observations remain, generic proof is refused.
	auto literal_bytes = auction_test_packet(6, 256);
	for (size_t slot = 0; slot < 6; ++slot)
	{
		std::vector<player_item_snapshot> root;
		for (size_t n = 0; n < 256; ++n)
		{
			auto item = auction_test_item(100 + slot * 10000 + n, n ? 0 : -1);
			item.name = std::string(1024, 'n');
			item.short_description = std::string(1024, 's');
			item.description = std::string(512, 'd');
			item.action_description = std::string(512, 'a');
			root.push_back(std::move(item));
		}
		auto blob = auction_test_blob(root);
		assert(blob.size() <= 1024 * 1024);
		literal_bytes.base.source2.tables[7].rows[slot].cells[5] = std::move(blob);
	}
	auto byte_refusal = auction_test_inspect(literal_bytes);
	assert(byte_refusal.nodes.size() == 1536 &&
	       auction_test_findings(byte_refusal, auction_physical_issue::native_bound_refused));
	for (const auto &node : byte_refusal.nodes)
		assert(node.generic_proof != auction_physical_generic_proof::observed_match);
	// Public codec depth32 accepts; depth33 refuses without guessed reason.
	items.clear();
	for (size_t n = 0; n < 33; ++n)
		items.push_back(auction_test_item(
			100 + n, n ? static_cast<int32_t>(n == 32 ? 0 : n - 1) : -1));
	std::string depth_blob = auction_test_blob(items);
	std::vector<player_item_snapshot> decoded;
	std::vector<uint64_t> revisions;
	assert(auction_repository_decode_native_tree_blob(
		{ reinterpret_cast<const uint8_t *>(depth_blob.data()), depth_blob.size() },
		&decoded, &revisions));
	const size_t literal_begin = 12 + items.size() * 16,
		     stride = (depth_blob.size() - literal_begin - 4) / items.size();
	assert(literal_begin + 4 + items.size() * stride == depth_blob.size());
	const size_t last_parent = literal_begin + 4 + 32 * stride;
	for (size_t byte = 0; byte < 4; ++byte)
		depth_blob[last_parent + byte] = static_cast<char>(uint32_t(31) >> (8 * byte));
	assert(!auction_repository_decode_native_tree_blob(
		{ reinterpret_cast<const uint8_t *>(depth_blob.data()), depth_blob.size() },
		&decoded, &revisions));
	auto depth = packet;
	depth.base.source2.tables[7].rows[0].cells[5] = depth_blob;
	assert(auction_test_findings(auction_test_inspect(depth),
				     auction_physical_issue::literal_decode_refused));
	auto capacity = packet;
	capacity.base.source2.tables[4].rows[0].cells[11] = std::string(1024 * 1024, 'x');
	assert(auction_test_inspect(capacity).nodes.size() == 2);
	capacity.base.source2.tables[4].rows[0].cells[11]->push_back('x');
	auction_test_reframe(capacity);
	auction_physical_correspondence sentinel;
	sentinel.diagnostic_count = 991;
	assert(auction_repository_inspect_physical_sources(capacity.base, capacity.supplement, {},
							   512, &sentinel) == E2BIG &&
	       sentinel.diagnostic_count == 991);
	// Single ACT2 native4096 accepts through the existing public decoder even
	// when its theoretical literal size exceeds the separate raw SQL cell bound.
	items.clear();
	for (size_t n = 0; n < 4096; ++n)
		items.push_back(auction_test_item(100 + n, n ? 0 : -1));
	std::string maximum_blob = auction_test_blob(items);
	assert(auction_repository_decode_native_tree_blob(
		       { reinterpret_cast<const uint8_t *>(maximum_blob.data()),
			 maximum_blob.size() },
		       &decoded, &revisions) &&
	       decoded.size() == 4096);
	for (size_t byte = 0; byte < 4; ++byte)
		maximum_blob[4 + byte] = static_cast<char>(uint32_t(4097) >> (8 * byte));
	assert(!auction_repository_decode_native_tree_blob(
		{ reinterpret_cast<const uint8_t *>(maximum_blob.data()), maximum_blob.size() },
		&decoded, &revisions));
	// Exact shared64MiB aggregate and one-byte refusal, with <=1MiB each cell.
	auto aggregate = packet;
	aggregate.base.source2.tables[4].rows[0].cells[11] = std::string(1024 * 1024, 'x');
	for (size_t n = 1; n < 64; ++n)
	{
		auto row = aggregate.base.source2.tables[4].rows[0];
		row.cells[0] = std::to_string(10 + n);
		row.cells[9] = std::nullopt;
		aggregate.base.source2.tables[4].rows.push_back(std::move(row));
	}
	auction_test_reframe(aggregate);
	const uint64_t aggregate_limit = 64 * 1024 * 1024;
	const auto metadata = aggregate.supplement.cell_bytes - aggregate_limit;
	assert(metadata > 0 && metadata < 1024 * 1024);
	aggregate.base.source2.tables[4].rows.back().cells[11]->resize(1024 * 1024 - metadata);
	auction_test_reframe(aggregate);
	assert(aggregate.supplement.cell_bytes == aggregate_limit);
	auction_physical_correspondence full;
	assert(!auction_repository_inspect_physical_sources(aggregate.base, aggregate.supplement,
							    {}, 512, &full));
	aggregate.base.source2.tables[4].rows.back().cells[11]->push_back('x');
	auction_test_reframe(aggregate);
	assert(auction_repository_inspect_physical_sources(aggregate.base, aggregate.supplement, {},
							   512, &sentinel) == E2BIG &&
	       sentinel.diagnostic_count == 991);
	auto invalid = packet;
	invalid.base.digest[0] ^= 1;
	assert(auction_repository_inspect_physical_sources(invalid.base, invalid.supplement, {},
							   512, &sentinel) &&
	       sentinel.diagnostic_count == 991);
	auction_test_reframe(packet);
	auction_fault_seen = 0;
	auction_fault_target = 0;
	auction_fault_enabled = true;
	auction_physical_correspondence counted;
	assert(!auction_repository_inspect_physical_sources(packet.base, packet.supplement, {}, 512,
							    &counted));
	auction_fault_enabled = false;
	const auto allocations = auction_fault_seen;
	for (size_t at = 1; at <= allocations; at += std::max(size_t{ 1 }, allocations / 100))
	{
		auction_physical_correspondence preserved;
		preserved.diagnostic_count = 991;
		auction_fault_seen = 0;
		auction_fault_target = at;
		auction_fault_enabled = true;
		const auto code = auction_repository_inspect_physical_sources(
			packet.base, packet.supplement, {}, 512, &preserved);
		auction_fault_enabled = false;
		if (code)
			assert(code == ENOMEM && preserved.diagnostic_count == 991);
		else
			assert(preserved.diagnostic_count != 991 &&
			       auction_test_findings(
				       preserved,
				       auction_physical_issue::original_provenance_unknown));
	}
}
[[maybe_unused]] void auction_physical_native_v1_readback(MYSQL *connection, uint32_t auction_id)
{
#ifndef __NO_MYSQL__
	using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	reconnect_flag reconnect = false;
	assert(!mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect));
	assert(!mysql_query(connection, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ"));
	assert(!mysql_query(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT"));
	const auto before = connection->server_status;
	assert(before & SERVER_STATUS_IN_TRANS);
	economic_sql_physical_source_snapshot base;
	sql_room_item_source_snapshot supplement;
	assert(!economic_sql_capture_physical_sources_in_transaction(connection, {}, &base));
	assert(!sql_room_item_payload_capture_sources_in_transaction(connection, {}, base,
								     &supplement));
	auction_physical_correspondence report;
	assert(!auction_repository_inspect_physical_sources(base, supplement, {}, 512, &report));
	assert(connection->server_status & SERVER_STATUS_IN_TRANS);
	auto found = std::find_if(report.listings.begin(), report.listings.end(),
				  [&](const auto &row) { return row.auction_id == auction_id; });
	assert(found != report.listings.end() &&
	       found->family == auction_physical_family::legacy_or_v1);
	for (const auto &slot : report.slots)
		if (slot.auction_id == auction_id)
			assert(slot.node_count == 0);
	assert(!mysql_query(connection, "ROLLBACK"));
#else
	(void)connection;
	(void)auction_id;
#endif
}
}
