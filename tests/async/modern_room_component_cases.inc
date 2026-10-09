// Extension of the original native payload fixture. Matching raw hashes here
// are manufactured component inputs, never database provenance or activation.
// Original shared framing helper is included once by the original room driver.

static economic_sql_physical_source_snapshot modern_base()
{
	economic_sql_physical_source_snapshot value;
#include "component_registry.inc"
	const std::string operation(16, 'x');
	component_row(value.source2.tables[12], { "3", "120", "0", "1" });
	component_row(value.source2.tables[17],
		      { operation, std::string(32, 'h'), std::string(32, 'k'), "5", "2", "4", "1",
			"0", "0", "1", std::string(8, '\0'), "1" });
	for (size_t index = 0; index < 3; ++index)
	{
		const auto uid = std::to_string(100 + index);
		component_row(value.source2.tables[11],
			      { uid, "100",
				index ? std::optional<std::string>(std::to_string(99 + index)) :
					std::nullopt,
				"3", "120", "0", std::to_string(11 + index), "1200", "1",
				std::nullopt });
		component_row(value.source2.item_equipment_sources[0], { uid, "0" });
	}
	reframe_physical_component(value);
	assert(!economic_sql_validate_physical_sources(value));
	return value;
}
static void modern_reframe(std::vector<economic_sql_source_table> &tables)
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
static std::vector<economic_sql_source_table> modern_tables()
{
	std::vector<economic_sql_source_table> tables;
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
		tables.push_back(component_table(spec[0], spec[1], spec[2]));
	const std::string operation(16, 'x');
	component_row(tables[1], { "1", "1", "active" });
	component_row(tables[4], { operation, "1", "0" });
	for (size_t index = 0; index < 3; ++index)
	{
		const auto uid = std::to_string(100 + index), revision = std::to_string(11 + index),
			   event = std::to_string(index);
		auto literal = item(100 + index, -1);
		std::vector<uint8_t> encoded;
		assert(player_item_snapshot_list_encode({ literal }, &encoded) ==
		       player_snapshot_codec_result::ok);
		component_row(tables[0],
			      { uid, revision, "1", operation, "1",
				std::string(reinterpret_cast<const char *>(encoded.data()),
					    encoded.size()) });
		component_row(
			tables[2],
			{ operation, event, uid, "100",
			  index ? std::optional<std::string>(std::to_string(99 + index)) :
				  std::nullopt,
			  "1", "7", "0", "3", "120", "0", revision, "1", "1",
			  std::to_string(static_cast<unsigned>(item_transfer_reason::player_drop)),
			  "120", "1", "0", "0" });
		component_row(tables[3],
			      { operation, event, event, "0", uid, std::to_string(10 + index),
				revision, operation, event });
	}
	modern_reframe(tables);
	return tables;
}
static sql_room_item_source_evidence
modern_inspect(const economic_sql_physical_source_snapshot &base,
	       const std::vector<economic_sql_source_table> &tables, size_t diagnostics = 512)
{
	sql_room_item_source_evidence result;
	assert(!sql_room_item_payload_inspect_sources(base, tables, {}, diagnostics, &result));
	return result;
}
static void modern_chain(size_t count, size_t dynamic_count, size_t extra_count,
			 economic_sql_physical_source_snapshot &base,
			 std::vector<economic_sql_source_table> &tables)
{
	base = modern_base();
	tables = modern_tables();
	base.source2.tables[11].rows.clear();
	base.source2.item_equipment_sources[0].rows.clear();
	for (size_t table : { size_t{ 0 }, size_t{ 2 }, size_t{ 3 } })
		tables[table].rows.clear();
	const std::string operation(16, 'x');
	for (size_t index = 0; index < count; ++index)
	{
		const auto uid = std::to_string(100 + index), event = std::to_string(index);
		const std::optional<std::string> parent =
			index ? std::optional<std::string>(std::to_string(99 + index)) :
				std::nullopt;
		component_row(base.source2.tables[11], { uid, "100", parent, "3", "120", "0", "11",
							 "1200", "1", std::nullopt });
		component_row(base.source2.item_equipment_sources[0], { uid, "0" });
		auto literal = item(100 + index, -1);
		literal.dynamic_affects.assign(dynamic_count, { 5, 13, 0 });
		literal.extra_descriptions.clear();
		for (size_t extra = 0; extra < extra_count; ++extra)
			literal.extra_descriptions.push_back(
				{ "literal", std::string(4000, 't'), false, {} });
		std::vector<uint8_t> encoded;
		assert(player_item_snapshot_list_encode({ literal }, &encoded) ==
		       player_snapshot_codec_result::ok);
		component_row(tables[0],
			      { uid, "11", "1", operation, "1",
				std::string(reinterpret_cast<const char *>(encoded.data()),
					    encoded.size()) });
		component_row(tables[2], { operation, event, uid, "100", parent, "1", "7", "0", "3",
					   "120", "0", "11", "1", "1",
					   std::to_string(static_cast<unsigned>(
						   item_transfer_reason::player_drop)),
					   "120", "1", "0", "0" });
		component_row(tables[3],
			      { operation, event, event, "0", uid, "10", "11", operation, event });
	}
	reframe_physical_component(base);
	modern_reframe(tables);
}
static void modern_room_component_cases()
{
	const auto base = modern_base();
	const auto tables = modern_tables();
	const auto complete = modern_inspect(base, tables);
	assert(complete.witnesses.size() == 3 && complete.graphs.size() == 1 &&
	       complete.graphs[0].valid && complete.diagnostics.empty());
	assert(complete.graphs[0].witness_indices == (std::vector<size_t>{ 0, 1, 2 }));
	for (const auto &witness : complete.witnesses)
		assert(witness.flags == SQL_ROOM_SOURCE_CURRENT && witness.literal &&
		       witness.ledger_row != SIZE_MAX && witness.reference_row != SIZE_MAX);
	for (size_t index = 0; index < 3; ++index)
	{
		auto missing = tables;
		missing[0].rows.erase(missing[0].rows.begin() + static_cast<ptrdiff_t>(index));
		modern_reframe(missing);
		const auto result = modern_inspect(base, missing);
		assert(result.witnesses.size() == 3 && !result.graphs[0].valid);
		assert(std::any_of(result.witnesses.begin(), result.witnesses.end(),
				   [](const auto &w)
				   { return w.flags & SQL_ROOM_SOURCE_MISSING_LITERAL; }));
	}
	{
		auto missing = tables;
		missing[0].rows.clear();
		modern_reframe(missing);
		const auto result = modern_inspect(base, missing);
		assert(result.witnesses.size() == 3 && result.graphs.size() == 1 &&
		       !result.graphs[0].valid);
		for (const auto &witness : result.witnesses)
			assert(witness.flags & SQL_ROOM_SOURCE_MISSING_LITERAL);
	}
	for (int change = 0; change < 5; ++change)
	{
		auto changed = base;
		auto raw = tables;
		if (change == 0)
		{
			changed.source2.tables[11].rows.clear();
			changed.source2.item_equipment_sources[0].rows.clear();
		}
		if (change == 1)
			for (auto &row : changed.source2.tables[11].rows)
				row.cells[3] = "1";
		if (change == 2)
			raw[1].rows[0].cells[1] = "2";
		if (change == 3)
			changed.source2.tables[12].rows.clear();
		if (change == 4)
			component_row(changed.physical_sources[2],
				      { "1", "synthetic_competitor", "120", std::nullopt, "100",
					"1200" });
		reframe_physical_component(changed);
		modern_reframe(raw);
		const auto result = modern_inspect(changed, raw);
		assert(!result.diagnostics.empty());
		if (change == 0)
			assert(result.witnesses[0].flags & SQL_ROOM_SOURCE_MISSING_CUSTODY);
		if (change == 1)
			assert((result.witnesses[0].flags & SQL_ROOM_SOURCE_MOVED_FROM_ROOM) &&
			       result.graphs.empty());
		if (change == 2)
			assert((result.witnesses[0].flags & SQL_ROOM_SOURCE_OLD_SEASON) &&
			       !(result.witnesses[0].flags & SQL_ROOM_SOURCE_CURRENT));
		if (change == 3)
			assert(result.witnesses[0].flags & SQL_ROOM_SOURCE_MISSING_OWNER_REVISION);
		if (change == 4)
			assert(result.witnesses[0].flags & SQL_ROOM_SOURCE_COMPETING_LEGACY);
	}
	// Both histories of one UID remain distinct immutable UID/revision records.
	{
		auto raw = tables;
		auto old = raw[0].rows[0];
		old.cells[1] = "9";
		old.cells[4] = "1";
		raw[0].rows.insert(raw[0].rows.begin(), old);
		modern_reframe(raw);
		const auto result = modern_inspect(base, raw);
		assert(result.witnesses.size() == 4 &&
		       (result.witnesses[0].flags & SQL_ROOM_SOURCE_HISTORY) &&
		       result.graphs[0].valid);
	}
	// A newer retained literal does not conceal older successful drop records
	// with missing UID/revision literals. Deduplicate malformed competing proof.
	{
		auto raw = tables;
		for (uint64_t revision : { uint64_t{ 7 }, uint64_t{ 9 } })
		{
			const std::string operation(16, static_cast<char>('a' + revision));
			auto entry = raw[2].rows[0];
			entry.cells[0] = operation;
			entry.cells[11] = std::to_string(revision);
			raw[2].rows.push_back(entry);
			component_row(raw[4], { operation, "1", "0" });
		}
		auto changed = base;
		for (uint64_t revision : { uint64_t{ 7 }, uint64_t{ 9 } })
			component_row(changed.source2.tables[17],
				      { std::string(16, static_cast<char>('a' + revision)),
					std::string(32, 'h'), std::string(32, 'k'), "5", "2", "4",
					"1", "0", "0", "1", std::string(8, '\0'), "1" });
		raw[2].rows.push_back(raw[2].rows.back());
		reframe_physical_component(changed);
		modern_reframe(raw);
		const auto result = modern_inspect(changed, raw);
		assert(result.witnesses.size() == 5 && result.graphs[0].valid);
		for (uint64_t revision : { uint64_t{ 7 }, uint64_t{ 9 } })
			assert(std::count_if(result.witnesses.begin(), result.witnesses.end(),
					     [&](const auto &w)
					     {
						     return w.item_uid == 100 &&
							    w.item_revision == revision &&
							    (w.flags & SQL_ROOM_SOURCE_HISTORY) &&
							    (w.flags &
							     SQL_ROOM_SOURCE_MISSING_LITERAL);
					     }) == 1);
	}
	for (int field : { 0, 6 })
	{
		auto changed = base;
		auto descendant = changed.source2.tables[11].rows[2];
		descendant.cells[0] = "103";
		descendant.cells[2] = "100";
		descendant.cells[field] = "invalid";
		changed.source2.tables[11].rows.push_back(descendant);
		component_row(changed.source2.item_equipment_sources[0],
			      { descendant.cells[0], "0" });
		reframe_physical_component(changed);
		const auto result = modern_inspect(changed, tables);
		assert(!result.graphs[0].valid && result.witnesses.size() == 4 &&
		       result.witnesses.back().custody_row == 3 &&
		       (result.witnesses.back().flags & SQL_ROOM_SOURCE_MALFORMED_IDENTITY));
	}
	for (int damage = 0; damage < 10; ++damage)
	{
		auto raw = tables;
		auto changed = base;
		if (damage == 0)
			raw[0].rows[0].cells[5] = std::string(4, '\0');
		if (damage == 1)
			raw[0].rows[0].cells[3] = std::string(16, 'z');
		if (damage == 2)
			raw[2].rows[0].cells[11] = "12";
		if (damage == 3)
			raw[3].rows[0].cells[5] = "8";
		if (damage == 4)
			changed.source2.tables[17].rows[0].cells[8] = "1";
		if (damage == 5)
			raw[4].rows[0].cells[1] = "2";
		if (damage == 6)
		{
			auto duplicate = raw[3].rows[0];
			duplicate.cells[1] = "9";
			raw[3].rows.push_back(duplicate);
		}
		if (damage == 7)
			changed.source2.tables[11].rows[1].cells[2] = "102";
		if (damage == 8)
			changed.source2.item_equipment_sources[0].rows[0].cells[1] = "7";
		if (damage == 9)
			raw[0].rows[0].cells[0] = std::string("100\0junk", 8);
		reframe_physical_component(changed);
		modern_reframe(raw);
		const auto result = modern_inspect(changed, raw);
		assert(!result.diagnostics.empty() && !result.graphs[0].valid);
	}
	for (int field = 0; field < 3; ++field)
	{
		economic_sql_source_limits limit;
		if (field == 0)
			limit.maximum_rows = base.rows;
		if (field == 1)
			limit.maximum_cells = base.cells;
		if (field == 2)
			limit.maximum_cell_bytes = base.cell_bytes;
		sql_room_item_source_evidence sentinel;
		sentinel.current_season = 987;
		assert(sql_room_item_payload_inspect_sources(base, tables, limit, 512, &sentinel) ==
			       E2BIG &&
		       sentinel.current_season == 987 && sentinel.witnesses.empty());
	}
	for (size_t diagnostics : { size_t{ 0 }, size_t{ 513 } })
	{
		sql_room_item_source_evidence sentinel;
		sentinel.current_season = 987;
		assert(sql_room_item_payload_inspect_sources(base, tables, {}, diagnostics,
							     &sentinel) == EINVAL &&
		       sentinel.current_season == 987);
	}
	{
		auto missing = tables;
		missing[0].rows.clear();
		modern_reframe(missing);
		const auto result = modern_inspect(base, missing, 1);
		assert(result.diagnostics.size() == 1 && result.diagnostics_truncated);
	}
	// Exact native depth and codec shared-row boundaries, no widened limits.
	for (size_t depth : { size_t{ 32 }, size_t{ 33 } })
	{
		economic_sql_physical_source_snapshot chain;
		std::vector<economic_sql_source_table> raw;
		modern_chain(depth, 0, 0, chain, raw);
		const auto result = modern_inspect(chain, raw);
		assert(result.graphs[0].valid == (depth == 32));
	}
	for (size_t rows : { size_t{ 8192 }, size_t{ 8193 } })
	{
		economic_sql_physical_source_snapshot chain;
		std::vector<economic_sql_source_table> raw;
		modern_chain(2, 4095, 0, chain, raw);
		if (rows == 8193)
		{
			auto literal = item(101, -1);
			literal.extra_descriptions.clear();
			literal.dynamic_affects.assign(4096, { 5, 13, 0 });
			std::vector<uint8_t> encoded;
			assert(player_item_snapshot_list_encode({ literal }, &encoded) ==
			       player_snapshot_codec_result::ok);
			raw[0].rows[1].cells[5] = std::string(
				reinterpret_cast<const char *>(encoded.data()), encoded.size());
			modern_reframe(raw);
		}
		assert(modern_inspect(chain, raw).graphs[0].valid == (rows == 8192));
	}
	{
		economic_sql_physical_source_snapshot chain;
		std::vector<economic_sql_source_table> raw;
		modern_chain(3, 0, 12, chain, raw);
		assert(!modern_inspect(chain, raw).graphs[0].valid); // >143072 graph bytes
		modern_chain(3001, 0, 0, chain, raw);
		assert(!modern_inspect(chain, raw).graphs[0].valid); // >3000 graph members
	}
#ifdef __NO_MYSQL__
	sql_room_item_source_snapshot sentinel;
	sentinel.rows = 987;
	assert(sql_room_item_payload_capture_sources_in_transaction(nullptr, {}, base, &sentinel) ==
		       ENOTSUP &&
	       sentinel.rows == 987);
#endif
}
