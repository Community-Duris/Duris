// Original room codec fixture extension. All framed inputs below are
// manufactured components; they establish no SQL provenance or activation.
static auction_component_packet union_packet()
{
	auto packet = auction_test_packet();
	auto collector = collector_physical_test::seed_base();
	for (size_t table : { size_t{ 8 }, size_t{ 9 }, size_t{ 10 } })
		packet.base.source2.tables[table].rows = collector.source2.tables[table].rows;
	packet.base.source2.tables[11].rows.push_back(collector.source2.tables[11].rows[0]);
	packet.base.source2.item_equipment_sources[0].rows.push_back(
		collector.source2.item_equipment_sources[0].rows[0]);
	packet.base.source2.tables[12].rows.push_back(collector.source2.tables[12].rows[0]);
	packet.base.source2.tables[13].rows = collector.source2.tables[13].rows;
	packet.base.source2.tables[13].rows[0].cells[1] = "30000";
	economic_sql_physical_source_snapshot room;
	std::vector<economic_sql_source_table> tables;
	modern_chain(3, 0, 0, room, tables);
	for (size_t index = 0; index < 3; ++index)
	{
		const auto uid = std::to_string(20000 + index);
		const std::optional<std::string> parent =
			index ? std::optional<std::string>(std::to_string(19999 + index)) :
				std::nullopt;
		room.source2.tables[11].rows[index].cells[0] = uid;
		room.source2.tables[11].rows[index].cells[1] = "20000";
		room.source2.tables[11].rows[index].cells[2] = parent;
		room.source2.item_equipment_sources[0].rows[index].cells[0] = uid;
		tables[0].rows[index].cells[0] = uid;
		std::vector<uint8_t> encoded;
		assert(player_item_snapshot_list_encode({ item(20000 + index, -1) }, &encoded) ==
		       player_snapshot_codec_result::ok);
		tables[0].rows[index].cells[5] =
			std::string(reinterpret_cast<const char *>(encoded.data()), encoded.size());
		tables[2].rows[index].cells[2] = uid;
		tables[2].rows[index].cells[3] = "20000";
		tables[2].rows[index].cells[4] = parent;
		tables[3].rows[index].cells[4] = uid;
		packet.base.source2.tables[11].rows.push_back(room.source2.tables[11].rows[index]);
		packet.base.source2.item_equipment_sources[0].rows.push_back(
			room.source2.item_equipment_sources[0].rows[index]);
	}
	packet.base.source2.tables[12].rows.insert(packet.base.source2.tables[12].rows.begin(),
						   room.source2.tables[12].rows[0]);
	packet.base.source2.tables[17].rows.push_back(room.source2.tables[17].rows[0]);
	packet.supplement.tables[0].rows = tables[0].rows;
	packet.supplement.tables[1].rows = tables[1].rows;
	for (size_t table : { size_t{ 2 }, size_t{ 3 }, size_t{ 4 } })
		packet.supplement.tables[table].rows.insert(
			packet.supplement.tables[table].rows.end(), tables[table].rows.begin(),
			tables[table].rows.end());
	auction_test_reframe(packet);
	return packet;
}
static economic_sql_persisted_correspondence union_inspect(auction_component_packet packet,
							   size_t diagnostics = 512)
{
	auction_test_reframe(packet);
	economic_sql_persisted_correspondence result;
	assert(economic_sql_normalize_persisted_correspondence(packet.base, packet.supplement, {},
							       diagnostics, &result) ==
	       economic_accounting_error::ok);
	assert(result.physical.physical_digest == packet.base.digest &&
	       result.collector.physical_digest == packet.base.digest &&
	       result.auction.physical_digest == packet.base.digest);
	assert(result.auction.validated_supplement_table_digests[0] ==
	       packet.supplement.tables[0].content_digest);
	return result;
}
static void union_unmatched(const economic_sql_persisted_correspondence &result,
			    std::initializer_list<size_t> rows)
{
	assert(result.unmatched_active_custody_indices == std::vector<size_t>(rows));
	for (const size_t row : rows)
		assert(result.custody[row].matches.empty());
	assert(result.multiply_matched_active_custody_indices.empty());
}
static void physical_provider_union_cases()
{
	auto packet = union_packet();
	economic_sql_normalized_physical_sources formerly;
	assert(economic_sql_normalize_physical_sources(packet.base, 512, &formerly) ==
	       economic_accounting_error::ok);
	assert(formerly.unmatched_active_custody_indices ==
	       std::vector<size_t>({ 0, 1, 2, 3, 4, 5 }));
	const auto result = union_inspect(packet);
	assert(result.custody.size() == 6 && result.room.graphs.size() == 1 &&
	       result.room.graphs[0].valid && result.room.witnesses.size() == 3 &&
	       result.auction.nodes.size() == 2 && result.collector.listings.size() == 1);
	union_unmatched(result, {});
	assert(result.physical.unmatched_active_custody_indices ==
	       formerly.unmatched_active_custody_indices);
	const std::array<economic_sql_persisted_provider, 6> expected = {
		economic_sql_persisted_provider::auction,
		economic_sql_persisted_provider::auction,
		economic_sql_persisted_provider::collector,
		economic_sql_persisted_provider::room,
		economic_sql_persisted_provider::room,
		economic_sql_persisted_provider::room
	};
	for (size_t row = 0; row < 6; ++row)
	{
		assert(result.custody[row].custody_index == row &&
		       result.custody[row].matches.size() == 1 &&
		       result.custody[row].matches[0].provider == expected[row]);
		assert(result.physical.source2.items[row].source.table == 11 &&
		       result.physical.source2.items[row].source.row == row);
	}
	assert(result.auction.nodes[0].custody->table == 11 &&
	       !result.auction.nodes[0].custody->equipment &&
	       !result.auction.nodes[0].custody->supplemental &&
	       result.auction.nodes[0].custody->row == 0);
	for (const auto &node : result.auction.nodes)
		assert(node.original_provenance_unknown);
	assert(!result.collector.listings[0].retained_command_proof_known);
	for (size_t row = 3; row < 6; ++row)
		assert(result.room.witnesses[result.custody[row].matches[0].witness_index]
			       .custody_row == row);
	// Capped provider detail does not erase the full witness inventories/findings.
	{
		auto damaged = packet;
		damaged.base.source2.tables[10].rows[0].cells[11] = std::nullopt;
		damaged.base.source2.tables[9].rows.clear();
		const auto report = union_inspect(damaged, 1);
		union_unmatched(report, { 2 });
		assert(report.collector.listings.size() == 1 &&
		       report.collector.custody.size() == 1 &&
		       report.collector.diagnostics.size() == 1 &&
		       report.collector.diagnostics_truncated &&
		       (report.collector.listings[0].findings & COLLECTOR_SOURCE_MISSING_LITERAL) &&
		       (report.collector.listings[0].findings & COLLECTOR_SOURCE_MISSING_DEATH));
	}
	// Claimed slot literals and terminal collector records remain history.
	{
		auto history = packet;
		history.base.source2.tables[7].rows[0].cells[8] = "1";
		history.base.source2.tables[7].rows[0].cells[6] = "7";
		history.base.source2.tables[7].rows[0].cells[7] = std::string(16, 'c');
		history.base.source2.tables[7].rows[0].cells[3] = "3";
		for (size_t row : { size_t{ 0 }, size_t{ 1 } })
		{
			history.base.source2.tables[11].rows[row].cells[3] = "1";
			history.base.source2.tables[11].rows[row].cells[4] = "7";
		}
		const auto report = union_inspect(history);
		union_unmatched(report, { 0, 1 });
		assert(report.auction.slots.size() == 1 && *report.auction.slots[0].claimed &&
		       report.auction.nodes.size() == 2);
		assert(!auction_test_findings(report.auction,
					      auction_physical_issue::history_metadata));
		for (const auto &node : report.auction.nodes)
			assert(!node.current_field_correspondence &&
			       node.original_provenance_unknown);
	}
	{
		auto history = packet;
		auto record = collector_physical_test::seed_record(1, 501);
		assert(collector::activate(&record, record.revision, record.sale_at) ==
		       collector::outcome::applied);
		assert(collector::purchase(&record, record.revision, 43, UINT64_MAX, true,
					   record.available_at) == collector::outcome::applied);
		const auto blob = history.base.source2.tables[10].rows[0].cells[11];
		history.base.source2.tables[10].rows.clear();
		collector_physical_test::append_record(history.base, record, blob);
		history.base.source2.tables[11].rows[2].cells[3] = "1";
		history.base.source2.tables[11].rows[2].cells[4] = "43";
		const auto report = union_inspect(history);
		union_unmatched(report, { 2 });
		assert(!report.collector.listings[0].held && report.collector.listings[0].literal &&
		       !report.collector.listings[0].retained_command_proof_known);
	}
	{
		auto history = packet;
		history.supplement.tables[1].rows[0].cells[1] = "2";
		const auto report = union_inspect(history);
		union_unmatched(report, { 3, 4, 5 });
		assert(report.room.witnesses.size() == 3 && report.room.graphs.empty());
	}
	for (int provider = 0; provider < 3; ++provider)
	{
		auto damaged = packet;
		if (provider == 0)
			damaged.base.source2.tables[7].rows[0].cells[5] = "ACT2-refused";
		if (provider == 1)
			damaged.base.source2.tables[10].rows[0].cells[11] = std::string(4, '\0');
		if (provider == 2)
			damaged.supplement.tables[0].rows[1].cells[5] = std::string(4, '\0');
		const auto report = union_inspect(damaged);
		if (provider == 0)
		{
			union_unmatched(report, { 0, 1 });
			assert(report.auction.slots.size() == 1 && report.auction.nodes.empty() &&
			       !report.auction.missing_literal_events.empty());
		}
		if (provider == 1)
		{
			union_unmatched(report, { 2 });
			assert(report.collector.listings[0].findings &
			       COLLECTOR_SOURCE_MALFORMED_LITERAL);
		}
		if (provider == 2)
		{
			union_unmatched(report, { 3, 4, 5 });
			assert(report.room.witnesses.size() == 3 && !report.room.graphs[0].valid);
		}
	}
	// Duplicate live candidates are retained with conflicts, never preferred.
	{
		auto duplicate = packet;
		duplicate.base.source2.tables[7].rows.push_back(
			duplicate.base.source2.tables[7].rows[0]);
		const auto report = union_inspect(duplicate);
		union_unmatched(report, { 0, 1 });
		assert(report.auction.slots.size() == 2 && report.auction.nodes.size() == 4 &&
		       auction_test_findings(report.auction,
					     auction_physical_issue::duplicate_current_uid));
	}
	// A reachable siege projection aliases the modern-room root. Both real
	// providers retain their independent references; the union must not prefer one.
	{
		auto overlapping = packet;
		overlapping.base.source2.item_sources[2].rows.push_back(
			{ { "1", "120", std::nullopt, "20000", "1200" }, {} });
		const auto report = union_inspect(overlapping);
		assert(report.custody.size() == 6 &&
		       report.unmatched_active_custody_indices.empty() &&
		       report.multiply_matched_active_custody_indices ==
			       std::vector<size_t>({ 3 }));
		assert(report.room.graphs.size() == 1 && report.room.graphs[0].valid &&
		       report.room.witnesses.size() == 3);
		assert(report.custody[3].matches.size() == 2);
		assert(report.custody[3].matches[0].provider ==
			       economic_sql_persisted_provider::physical &&
		       report.custody[3].matches[1].provider ==
			       economic_sql_persisted_provider::room);
		for (size_t row :
		     { size_t{ 0 }, size_t{ 1 }, size_t{ 2 }, size_t{ 4 }, size_t{ 5 } })
			assert(report.custody[row].matches.size() == 1 &&
			       report.custody[row].matches[0].provider == expected[row]);
		assert(report.physical.source2.items[3].source.row == 3 &&
		       report.room.witnesses[report.custody[3].matches[1].witness_index]
				       .custody_row == 3 &&
		       report.physical.items[report.custody[3].matches[0].witness_index]
				       .source.registry ==
			       economic_sql_physical_registry::source2_item_sources &&
		       report.physical.items[report.custody[3].matches[0].witness_index]
				       .source.table == 2 &&
		       report.physical.items[report.custody[3].matches[0].witness_index]
				       .source.row == 0 &&
		       overlapping.base.source2.item_sources[2].rows.size() == 1);
	}
	// Duplicate custody PK/numeric failures are upstream refusals, not a partial report.
	for (int failure = 0; failure < 11; ++failure)
	{
		auto damaged = packet;
		economic_sql_source_limits limits;
		size_t diagnostics = 512;
		if (failure == 0)
			damaged.base.digest[0] ^= 1;
		if (failure == 1)
		{
			damaged.base.source2.tables[11].rows.push_back(
				damaged.base.source2.tables[11].rows.back());
			damaged.base.source2.item_equipment_sources[0].rows.push_back(
				damaged.base.source2.item_equipment_sources[0].rows.back());
			auction_test_reframe(damaged);
		}
		if (failure == 2)
			diagnostics = 0;
		if (failure == 3)
			diagnostics = 513;
		if (failure == 4)
			limits.maximum_rows = damaged.base.rows - 1;
		if (failure == 5)
			limits.maximum_cells = damaged.base.cells - 1;
		if (failure == 6)
			limits.maximum_cell_bytes = damaged.base.cell_bytes - 1;
		if (failure == 7)
			damaged.supplement.tables[0].rows[0].digest[0] ^= 1;
		// Base itself fits; the five supplemental projections must share its budget.
		if (failure == 8)
			limits.maximum_rows = damaged.base.rows;
		if (failure == 9)
			limits.maximum_cells = damaged.base.cells;
		if (failure == 10)
			limits.maximum_cell_bytes = damaged.base.cell_bytes;
		economic_sql_persisted_correspondence output;
		output.collector.next_listing = 987;
		output.unmatched_active_custody_indices = { 987 };
		assert(economic_sql_normalize_persisted_correspondence(
			       damaged.base, damaged.supplement, limits, diagnostics, &output) !=
			       economic_accounting_error::ok &&
		       output.collector.next_listing == 987 && output.custody.empty() &&
		       output.unmatched_active_custody_indices == std::vector<size_t>({ 987 }));
	}
	// Original allocator fault policy: operational errors preserve output;
	// a bool-only native auction decoder may instead retain unknown refusal.
	auction_fault_seen = auction_fault_target = 0;
	auction_fault_enabled = true;
	economic_sql_persisted_correspondence counted;
	const auto count_code = economic_sql_normalize_persisted_correspondence(
		packet.base, packet.supplement, {}, 512, &counted);
	auction_fault_enabled = false;
	assert(count_code == economic_accounting_error::ok);
	const size_t allocations = auction_fault_seen;
	assert(allocations);
	for (size_t at = 1; at <= allocations; at += std::max(size_t{ 1 }, allocations / 100))
	{
		economic_sql_persisted_correspondence output;
		output.collector.next_listing = 987;
		auction_fault_seen = 0;
		auction_fault_target = at;
		auction_fault_enabled = true;
		const auto code = economic_sql_normalize_persisted_correspondence(
			packet.base, packet.supplement, {}, 512, &output);
		auction_fault_enabled = false;
		assert(auction_fault_seen >= at);
		if (code != economic_accounting_error::ok)
			assert(code == economic_accounting_error::capacity &&
			       output.collector.next_listing == 987 && output.custody.empty());
		else
		{
			assert(output.collector.next_listing == result.collector.next_listing &&
			       output.custody.size() == 6 &&
			       output.collector.listings.size() == 1 &&
			       output.collector.listings[0].correspondence_valid &&
			       output.room.graphs.size() == 1 && output.room.graphs[0].valid &&
			       output.room.witnesses.size() == 3);
			for (size_t row : { size_t{ 2 }, size_t{ 3 }, size_t{ 4 }, size_t{ 5 } })
				assert(output.custody[row].custody_index == row &&
				       output.custody[row].matches.size() == 1 &&
				       output.custody[row].matches[0].provider == expected[row]);
			if (output.unmatched_active_custody_indices.empty())
			{
				union_unmatched(output, {});
				assert(output.auction.nodes.size() == 2);
				for (size_t row : { size_t{ 0 }, size_t{ 1 } })
					assert(output.custody[row].matches.size() == 1 &&
					       output.custody[row].matches[0].provider ==
						       expected[row]);
			}
			else
			{
				union_unmatched(output, { 0, 1 });
				assert(auction_test_findings(
					output.auction,
					auction_physical_issue::literal_decode_refused));
			}
			for (const auto &node : output.auction.nodes)
				assert(node.original_provenance_unknown);
			assert(!output.collector.listings[0].retained_command_proof_known);
		}
	}
}
