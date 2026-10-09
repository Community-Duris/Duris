// Additive original auction/union component controls. These framed modeled
// inputs are not SQL/native execution or original provenance evidence.
namespace
{
auction_component_packet auction_legacy_packet(size_t roots = 1)
{
	auto packet = auction_test_packet(roots, 1);
	packet.base.source2.tables[17].rows[0].cells[4] = "1";
	packet.base.source2.tables[17].rows[0].cells[5] = "1";
	packet.base.source2.tables[4].rows[0].cells[11] = "x";
	for (auto &row : packet.base.source2.tables[7].rows)
		row.cells[5] = "x";
	auction_test_reframe(packet);
	return packet;
}
void auction_legacy_identity_component_cases()
{
	auto packet = auction_legacy_packet();
	auto observed = auction_test_inspect(packet);
	assert(observed.nodes.empty() && observed.legacy_identities.size() == 1);
	assert(observed.listings[0].legacy_listing_receipt);
	const auto &identity = observed.legacy_identities[0];
	assert(identity.current_field_correspondence && identity.item_uid == 100 &&
	       identity.listing_after_revision == 2 && identity.vnum == 77 && identity.custody &&
	       identity.ledger &&
	       identity.ownership_ledger_proof == auction_physical_generic_proof::observed_match &&
	       identity.decoded_content_unknown && identity.original_provenance_unknown);
	assert(observed.unmatched_auction_custody.empty());
	assert(observed.slots[0].legacy_identity_index == 0 && observed.slots[0].node_count == 0);
	// Equal common-blob change is observable, but no original content proof exists.
	auto changed_content = packet;
	changed_content.base.source2.tables[4].rows[0].cells[11] = "changed common opaque bytes";
	changed_content.base.source2.tables[7].rows[0].cells[5] = "changed common opaque bytes";
	auto changed_identity = auction_test_inspect(changed_content);
	assert(changed_identity.legacy_identities[0].current_field_correspondence &&
	       changed_identity.legacy_identities[0].decoded_content_unknown &&
	       changed_identity.slots[0].source.digest != observed.slots[0].source.digest);
	// Explicit distinct union domain, without any fabricated decoded object.
	economic_sql_persisted_correspondence union_report;
	assert(economic_sql_normalize_persisted_correspondence(packet.base, packet.supplement, {},
							       512, &union_report) ==
	       economic_accounting_error::ok);
	assert(union_report.custody.size() == 1 && union_report.custody[0].matches.size() == 1 &&
	       union_report.custody[0].matches[0].provider ==
		       economic_sql_persisted_provider::auction_legacy_identity &&
	       union_report.unmatched_active_custody_indices.empty());
	for (int mode = 0; mode < 2; ++mode)
	{
		auto unmatched = packet;
		if (mode == 0)
			unmatched.base.source2.tables[11].rows[0].cells[7] = "78";
		if (mode == 1)
			unmatched.supplement.tables[2].rows.clear();
		auction_test_reframe(unmatched);
		economic_sql_persisted_correspondence result;
		assert(economic_sql_normalize_persisted_correspondence(
			       unmatched.base, unmatched.supplement, {}, 512, &result) ==
		       economic_accounting_error::ok);
		assert(result.custody[0].matches.empty() &&
		       result.unmatched_active_custody_indices.size() == 1);
	}
	for (int mode = 0; mode < 16; ++mode)
	{
		auto bad = packet;
		auto &parent = bad.base.source2.tables[4].rows[0].cells;
		auto &slot = bad.base.source2.tables[7].rows[0].cells;
		if (mode == 0)
			parent[9] = std::nullopt;
		if (mode == 1)
			slot[5] = "changed opaque bytes";
		if (mode == 2)
			slot[3] = "3";
		if (mode == 3)
			bad.supplement.tables[2].rows.clear();
		if (mode == 4)
			bad.supplement.tables[2].rows[0].cells[16] = "65535";
		if (mode == 5)
			bad.base.source2.tables[11].rows.push_back(
				bad.base.source2.tables[11].rows[0]);
		if (mode == 6)
			component_row(bad.base.source2.tables[11],
				      { "101", "100", "100", "1", "7", "0", "2", "77", "1",
					std::nullopt });
		if (mode == 7)
			bad.base.source2.tables[11].rows[0].cells[2] = "100";
		if (mode == 8)
			bad.base.source2.item_equipment_sources[0].rows[0].cells[1] = "1";
		if (mode == 9)
			bad.base.source2.tables[12].rows[0].cells[3] = "0";
		if (mode == 10)
			bad.base.source2.tables[17].rows[0].cells[10]->back() = 1;
		if (mode == 11)
			bad.base.source2.tables[17].rows.push_back(
				bad.base.source2.tables[17].rows[0]);
		if (mode == 12)
			bad.base.source2.tables[7].rows.push_back(
				bad.base.source2.tables[7].rows[0]);
		if (mode == 13)
			bad.base.source2.tables[11].rows[0].cells[0] = "malformed";
		if (mode == 14)
			component_row(bad.base.source2.tables[11],
				      { "101", "999", "100", "1", "7", "0", "2", "77", "1",
					std::nullopt });
		if (mode == 15)
		{
			const auto vnum = "-1";
			parent[10] = vnum;
			slot[4] = vnum;
			bad.base.source2.tables[11].rows[0].cells[7] = vnum;
		}
		auto refused = auction_test_inspect(bad, 1);
		for (const auto &witness : refused.legacy_identities)
			assert(!witness.current_field_correspondence);
		assert(!refused.unmatched_auction_custody.empty());
		if (mode == 6 || mode == 14)
			assert(refused.extra_custody.size() == 1);
	}
	// Original auction command validator accepts VNUM zero; do not narrow it.
	auto zero_vnum = packet;
	zero_vnum.base.source2.tables[4].rows[0].cells[10] = "0";
	zero_vnum.base.source2.tables[7].rows[0].cells[4] = "0";
	zero_vnum.base.source2.tables[11].rows[0].cells[7] = "0";
	assert(auction_test_inspect(zero_vnum).legacy_identities[0].current_field_correspondence);
	auto staged = packet;
	staged.base.source2.tables[7].rows[0].cells[6] = "8";
	assert(auction_test_inspect(staged).legacy_identities[0].current_field_correspondence);
	auto multiple = auction_legacy_packet(2);
	// Original v1 common first-root blob need not encode each later root's UID.
	multiple.base.source2.tables[7].rows[1].cells[4] = "78";
	multiple.base.source2.tables[11].rows[1].cells[7] = "78";
	auto two = auction_test_inspect(multiple);
	assert(two.legacy_identities.size() == 2 && two.nodes.empty());
	for (const auto &witness : two.legacy_identities)
		assert(witness.current_field_correspondence);
	// Claimed retained UID can coexist with a later live listing of the same UID.
	auto history = packet;
	auto old_parent = history.base.source2.tables[4].rows[0];
	old_parent.cells[0] = "9";
	old_parent.cells[9] = std::string(16, 'b');
	history.base.source2.tables[4].rows.insert(history.base.source2.tables[4].rows.begin(),
						   old_parent);
	auto old_input = history.base.source2.tables[17].rows[0];
	old_input.cells[0] = old_parent.cells[9];
	auction_command_result old_result{};
	assert(auction_command_decode_result(
		reinterpret_cast<const uint8_t *>(old_input.cells[10]->data()),
		old_input.cells[10]->size(), &old_result));
	old_result.auction_id = 9;
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded{};
	assert(auction_command_encode_result(old_result, &encoded));
	old_input.cells[10] =
		std::string(reinterpret_cast<const char *>(encoded.data()), encoded.size());
	history.base.source2.tables[17].rows.push_back(old_input);
	auto old_slot = history.base.source2.tables[7].rows[0];
	old_slot.cells[0] = "9";
	old_slot.cells[3] = "3";
	old_slot.cells[6] = "8";
	old_slot.cells[7] = std::string(16, 'c');
	old_slot.cells[8] = "1";
	history.base.source2.tables[7].rows.insert(history.base.source2.tables[7].rows.begin(),
						   old_slot);
	auto retained = auction_test_inspect(history);
	assert(retained.legacy_identities.size() == 2 &&
	       !retained.legacy_identities[0].current_field_correspondence &&
	       retained.legacy_identities[1].current_field_correspondence &&
	       !auction_test_findings(retained, auction_physical_issue::duplicate_current_uid));
	// Positive-quantity legacy parent with no stable per-slot identity is a finding.
	auto missing = packet;
	missing.base.source2.tables[7].rows.clear();
	missing.base.source2.tables[4].rows[0].cells[8] = "0";
	missing.base.source2.tables[4].rows[0].cells[9] = std::nullopt;
	missing.base.source2.tables[4].rows[0].cells[10] = "0";
	missing.base.source2.tables[4].rows[0].cells[11] = "";
	auto no_identity = auction_test_inspect(missing, 1);
	assert(no_identity.legacy_identities.empty() &&
	       auction_test_findings(no_identity, auction_physical_issue::missing_identity) == 1 &&
	       no_identity.listings.size() == 1);
	// Partial opaque parent cannot hide a second declared item behind claim history.
	auto partial = packet;
	partial.base.source2.tables[4].rows[0].cells[6] = "2";
	partial.base.source2.tables[4].rows[0].cells[9] = std::nullopt;
	partial.base.source2.tables[7].rows[0].cells[8] = "1";
	partial.base.source2.tables[7].rows[0].cells[3] = "3";
	partial.base.source2.tables[7].rows[0].cells[6] = "8";
	partial.base.source2.tables[7].rows[0].cells[7] = std::string(16, 'c');
	partial.base.source2.tables[11].rows.clear();
	partial.base.source2.item_equipment_sources[0].rows.clear();
	auto incomplete = auction_test_inspect(partial, 1);
	assert(incomplete.listings.size() == 1 && incomplete.slots.size() == 1 &&
	       auction_test_findings(incomplete, auction_physical_issue::invalid_slots) &&
	       incomplete.legacy_identities.empty());
	// Structural/own allocation refusal must preserve caller output.
	auto corrupt = packet;
	corrupt.base.digest[0] ^= 1;
	auction_physical_correspondence sentinel;
	sentinel.diagnostic_count = 998;
	assert(auction_repository_inspect_physical_sources(corrupt.base, corrupt.supplement, {},
							   512, &sentinel) &&
	       sentinel.diagnostic_count == 998);
	auction_fault_seen = 0;
	auction_fault_target = 1;
	auction_fault_enabled = true;
	const auto code = auction_repository_inspect_physical_sources(
		packet.base, packet.supplement, {}, 512, &sentinel);
	auction_fault_enabled = false;
	assert(auction_fault_seen == 1 && code == ENOMEM && sentinel.diagnostic_count == 998);
}
}

// Genuine original v1 producer readback, no replacement command or blob codec.
namespace
{
[[maybe_unused]] void auction_legacy_identity_native_readback(MYSQL *connection, uint32_t auction_id)
{
#ifndef __NO_MYSQL__
	assert(!mysql_query(connection, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ"));
	assert(!mysql_query(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT"));
	const auto session = mysql_thread_id(connection);
	economic_sql_physical_source_snapshot base;
	sql_room_item_source_snapshot supplement;
	assert(!economic_sql_capture_physical_sources_in_transaction(connection, {}, &base));
	assert(!sql_room_item_payload_capture_sources_in_transaction(connection, {}, base,
								     &supplement));
	auction_physical_correspondence report;
	assert(!auction_repository_inspect_physical_sources(base, supplement, {}, 512, &report));
	size_t matched = 0;
	for (const auto &identity : report.legacy_identities)
	{
		const auto &slot = report.slots[identity.slot_index];
		if (slot.auction_id == auction_id)
		{
			assert(slot.node_count == 0 && identity.decoded_content_unknown &&
			       identity.original_provenance_unknown);
			assert(identity.current_field_correspondence && identity.custody &&
			       identity.ledger);
			++matched;
		}
	}
	assert(matched == 1 && mysql_thread_id(connection) == session &&
	       (connection->server_status & SERVER_STATUS_IN_TRANS));
	assert(!mysql_query(connection, "ROLLBACK"));
#else
	(void)connection;
	(void)auction_id;
#endif
}
}
