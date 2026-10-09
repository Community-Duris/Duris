// Exact native ceilings, using canonical complete codec payloads. These
// manufactured DTO cases prove report semantics only, never native provenance.
static void modern_room_explicit_boundary_cases()
{
	for (size_t roots : { size_t{ 4096 }, size_t{ 4097 } })
	{
		economic_sql_physical_source_snapshot base;
		std::vector<economic_sql_source_table> tables;
		modern_chain(roots, 0, 0, base, tables);
		for (size_t index = 0; index < roots; ++index)
		{
			const auto uid = std::to_string(100 + index);
			base.source2.tables[11].rows[index].cells[1] = uid;
			base.source2.tables[11].rows[index].cells[2] = std::nullopt;
			tables[2].rows[index].cells[3] = uid;
			tables[2].rows[index].cells[4] = std::nullopt;
		}
		reframe_physical_component(base);
		modern_reframe(tables);
		const auto result = modern_inspect(base, tables);
		assert(result.witnesses.size() == roots && result.graphs.size() == roots);
		assert(result.native_root_limit_exceeded == (roots == 4097));
		// An individually valid tree does not certify the complete root census.
		for (const auto &graph : result.graphs)
			assert(graph.valid && graph.witness_indices.size() == 1);
		assert(std::any_of(result.diagnostics.begin(), result.diagnostics.end(),
				   [](const auto &diagnostic) {
					   return diagnostic.flags &
						  SQL_ROOM_SOURCE_NATIVE_ROOT_LIMIT;
				   }) == (roots == 4097));
		if (roots == 4096)
			assert(result.diagnostics.empty());
	}
	for (size_t bytes : { size_t{ 131072 }, size_t{ 131073 } })
	{
		economic_sql_physical_source_snapshot base;
		std::vector<economic_sql_source_table> tables;
		modern_chain(1, 0, 0, base, tables);
		auto literal = item(100, -1);
		literal.dynamic_affects.clear();
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
		// The over-limit control remains canonical codec data: rejection must
		// come from the original room item limit, not trailing junk or corruption.
		std::vector<player_item_snapshot> decoded;
		assert(player_item_snapshot_list_decode(encoded.data(), encoded.size(), &decoded) ==
			       player_snapshot_codec_result::ok &&
		       decoded.size() == 1);
		tables[0].rows[0].cells[5] =
			std::string(reinterpret_cast<const char *>(encoded.data()), encoded.size());
		modern_reframe(tables);
		const auto result = modern_inspect(base, tables);
		assert(result.witnesses.size() == 1 && result.graphs.size() == 1);
		assert(result.graphs[0].valid == (bytes == 131072));
		assert(static_cast<bool>(result.witnesses[0].flags &
					 SQL_ROOM_SOURCE_MALFORMED_LITERAL) == (bytes == 131073));
		assert(result.witnesses[0].literal.has_value() == (bytes == 131072));
		if (bytes == 131072)
			assert(result.diagnostics.empty());
	}
}
