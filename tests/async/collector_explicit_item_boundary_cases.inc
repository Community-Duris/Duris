// Canonical complete item codec data at the collector's native byte ceiling.
// Manufactured EPH1 inputs test correspondence semantics, not SQL provenance.
static void collector_explicit_item_boundary_cases()
{
	for (const size_t bytes : { size_t{ 131072 }, size_t{ 131073 } })
	{
		auto base = collector_physical_test::seed_base();
		auto literal = collector_physical_test::seed_item(501);
		literal.extra_descriptions.assign(40, { "", "", false, {} });
		std::vector<uint8_t> encoded;
		assert(player_item_snapshot_list_encode({ literal }, &encoded) ==
		       player_snapshot_codec_result::ok);
		assert(encoded.size() < bytes);
		size_t remaining = bytes - encoded.size();
		for (auto &description : literal.extra_descriptions)
		{
			const auto length = std::min(remaining, PLAYER_SNAPSHOT_MAX_STRING_BYTES);
			description.description.assign(length, 'b');
			remaining -= length;
		}
		assert(!remaining);
		assert(player_item_snapshot_list_encode({ literal }, &encoded) ==
			       player_snapshot_codec_result::ok &&
		       encoded.size() == bytes);
		// The over-limit value is also canonical data, not trailing bytes or corruption.
		std::vector<player_item_snapshot> decoded;
		assert(player_item_snapshot_list_decode(encoded.data(), encoded.size(), &decoded) ==
			       player_snapshot_codec_result::ok &&
		       decoded.size() == 1 && decoded[0].object_uid == 501 &&
		       decoded[0].vnum == 1200 &&
		       decoded[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
		       !decoded[0].equipment_slot && !decoded[0].string_mask);
		std::vector<uint8_t> canonical;
		assert(player_item_snapshot_list_encode(decoded, &canonical) ==
			       player_snapshot_codec_result::ok &&
		       canonical == encoded);
		base.source2.tables[10].rows[0].cells[11] =
			std::string(reinterpret_cast<const char *>(encoded.data()), encoded.size());
		reframe_physical_component(base);
		assert(!economic_sql_validate_physical_sources(base));
		const auto source_digest = base.digest;
		collector_physical_source_report result;
		assert(!collector_repository_inspect_physical_sources(base, &result));
		assert(base.digest == source_digest &&
		       base.source2.tables[10].rows[0].cells[11]->size() == bytes &&
		       result.physical_digest == base.digest && result.rows == base.rows &&
		       result.cells == base.cells && result.cell_bytes == base.cell_bytes);
		assert(result.listings.size() == 1 && result.listings[0].listing_row == 0 &&
		       result.listings[0].held && result.custody.size() == 1 &&
		       result.listings[0].correspondence_valid == (bytes == 131072) &&
		       result.correspondence_valid == (bytes == 131072));
		assert(bool(result.listings[0].findings & COLLECTOR_SOURCE_MALFORMED_LITERAL) ==
			       (bytes == 131073) &&
		       result.listings[0].literal.has_value() == (bytes == 131072) &&
		       !result.listings[0].retained_command_proof_known);
		if (bytes == 131072)
			assert(result.diagnostics.empty() &&
			       result.listings[0].literal->extra_descriptions.size() == 40 &&
			       !result.listings[0].literal->string_mask);
		else
			assert(std::any_of(result.diagnostics.begin(), result.diagnostics.end(),
					   [](const auto &finding) {
						   return finding.findings &
							  COLLECTOR_SOURCE_MALFORMED_LITERAL;
					   }));
	}
}
