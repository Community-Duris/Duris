// Additive ORIGINAL union fixture components. Inputs remain manufactured
// framing/value evidence, never SQL, world-initialization or activation proof.
static void runtime_cache_correspondence_cases()
{
	const auto persisted = union_inspect(union_packet());
	std::vector<item_ownership_runtime_entry> cache;
	std::vector<std::optional<uint64_t>> clocks;
	for (const auto &native : persisted.physical.source2.items)
	{
		assert(native.owner_revision);
		const auto &position = native.item.position;
		cache.push_back({ native.item.uid, position.root_uid, position.parent_uid,
				  position.owner, position.revision, *native.owner_revision,
				  native.vnum, position.state });
		clocks.push_back(native.owner_revision);
	}
	auto inspect = [&](const auto &rows, const auto &observed, const auto &source,
			   size_t details = 512)
	{
		economic_sql_runtime_cache_correspondence report;
		assert(economic_sql_compare_runtime_cache(rows, observed, source, {}, details,
							  &report) ==
		       economic_accounting_error::ok);
		return report;
	};
	auto has = [](const auto &report, economic_sql_cache_issue issue)
	{ return report.issue_counts[size_t(issue)] > 0; };
	const auto exact = inspect(cache, clocks, persisted);
	assert(exact.findings.empty() && exact.cache.size() == 6 && exact.custody.size() == 6);
	for (const auto &row : exact.cache)
		assert(row.exact_current_correspondence && row.custody_index &&
		       row.sql_owner_index);
	// Equal malformed cache/SQL values cannot become exact merely by equality.
	// Native command935-936 and actual native custody predicate3451 retain these
	// UID/revision bounds; this is a manufactured pure-value defect control.
	for (size_t field = 0; field < 5; ++field)
	{
		auto malformed = cache;
		auto raw = persisted;
		if (field == 0)
			malformed[0].item_uid = raw.physical.source2.items[0].item.uid = UINT64_MAX;
		if (field == 1)
			malformed[0].root_item_uid =
				raw.physical.source2.items[0].item.position.root_uid = UINT64_MAX;
		if (field == 2)
			malformed[0].parent_item_uid =
				raw.physical.source2.items[0].item.position.parent_uid = UINT64_MAX;
		if (field == 3)
			malformed[0].item_revision =
				raw.physical.source2.items[0].item.position.revision = 0;
		if (field == 4)
			malformed[0].item_revision =
				raw.physical.source2.items[0].item.position.revision = UINT64_MAX;
		const auto report = inspect(malformed, clocks, raw);
		assert(has(report, economic_sql_cache_issue::invalid_cache) &&
		       !report.cache[0].exact_current_correspondence);
	}
	// Unloaded durable domains have zero cache observations, not a cache defect.
	const auto unloaded = inspect(std::vector<item_ownership_runtime_entry>{},
				      std::vector<std::optional<uint64_t>>{}, persisted);
	assert(unloaded.findings.empty() && unloaded.custody.size() == 6);
	for (const auto &row : unloaded.custody)
		assert(!row.active_cache_count && row.current_provider_count == 1);
	auto sibling = cache;
	for (auto &entry : sibling)
		if (entry.owner_revision)
			--entry.owner_revision;
	assert(inspect(sibling, clocks, persisted).findings.empty());
	auto future = cache;
	future[0].owner_revision = *clocks[0] + 1;
	assert(has(inspect(future, clocks, persisted),
		   economic_sql_cache_issue::future_entry_clock));
	auto wrong_clock = clocks;
	wrong_clock[0] = *clocks[0] + 1;
	assert(has(inspect(cache, wrong_clock, persisted),
		   economic_sql_cache_issue::current_owner_clock_mismatch));
	wrong_clock[0].reset();
	assert(has(inspect(cache, wrong_clock, persisted),
		   economic_sql_cache_issue::missing_runtime_owner_clock));
	for (size_t field = 0; field < 6; ++field)
	{
		auto changed = cache;
		if (field == 0)
			++changed[0].root_item_uid;
		if (field == 1)
			++changed[0].parent_item_uid;
		if (field == 2)
			changed[0].owner = { item_owner_type::room, 999, 0 };
		if (field == 3)
			++changed[0].item_revision;
		if (field == 4)
			++changed[0].vnum;
		if (field == 5)
			changed[0].state = item_custody_state::destroyed;
		const auto report = inspect(changed, clocks, persisted);
		assert(has(report, field == 5 ? economic_sql_cache_issue::inactive_cache :
						economic_sql_cache_issue::fields_mismatch));
		assert(!report.cache[0].exact_current_correspondence);
	}
	auto unknown = cache;
	unknown[0].item_uid = 40000;
	assert(has(inspect(unknown, clocks, persisted),
		   economic_sql_cache_issue::missing_persisted_uid));
	auto duplicate = cache;
	auto duplicate_clocks = clocks;
	duplicate.push_back(cache[0]);
	duplicate_clocks.push_back(clocks[0]);
	const auto duplicate_report = inspect(duplicate, duplicate_clocks, persisted);
	assert(has(duplicate_report, economic_sql_cache_issue::duplicate_cache_uid) &&
	       duplicate_report.cache.size() == 7 &&
	       duplicate_report.custody[0].active_cache_count == 2);
	auto damaged = persisted;
	damaged.physical.source2.owners.clear();
	assert(has(inspect(cache, clocks, damaged),
		   economic_sql_cache_issue::missing_sql_owner_clock));
	damaged = persisted;
	damaged.physical.source2.owners.push_back(damaged.physical.source2.owners[0]);
	assert(has(inspect(cache, clocks, damaged),
		   economic_sql_cache_issue::duplicate_sql_owner_clock));
	damaged = persisted;
	damaged.custody[0].matches.clear();
	assert(has(inspect(cache, clocks, damaged), economic_sql_cache_issue::missing_provider));
	damaged = persisted;
	damaged.custody[0].matches.push_back(damaged.custody[0].matches[0]);
	const auto multiple = inspect(cache, clocks, damaged, 1);
	assert(has(multiple, economic_sql_cache_issue::multiple_providers) &&
	       multiple.custody[0].current_provider_count == 2 &&
	       multiple.diagnostics.size() == 1 && multiple.findings.size() > 1 &&
	       multiple.diagnostics_truncated);
	// Retain all duplicate persisted rows without multiplying cache-row evidence.
	damaged = persisted;
	damaged.physical.source2.items.push_back(damaged.physical.source2.items[0]);
	damaged.custody.push_back(damaged.custody[0]);
	damaged.custody.back().custody_index = 6;
	const auto duplicate_sql = inspect(cache, clocks, damaged);
	assert(has(duplicate_sql, economic_sql_cache_issue::duplicate_persisted_uid) &&
	       duplicate_sql.custody.size() == 7 &&
	       duplicate_sql.custody[0].active_cache_count == 1 &&
	       duplicate_sql.custody.back().active_cache_count == 1);
	const auto duplicate_both = inspect(duplicate, duplicate_clocks, damaged);
	assert(has(duplicate_both, economic_sql_cache_issue::duplicate_cache_uid) &&
	       has(duplicate_both, economic_sql_cache_issue::duplicate_persisted_uid) &&
	       duplicate_both.custody[0].active_cache_count == 2 &&
	       duplicate_both.custody.back().active_cache_count == 2);
	damaged = persisted;
	auto history = damaged.physical.source2.items[0];
	history.item.uid = 40000;
	history.item.position.state = item_custody_state::destroyed;
	damaged.physical.source2.items.push_back(history);
	damaged.custody.push_back({ 6, {} });
	const auto historical = inspect(cache, clocks, damaged);
	assert(historical.findings.empty() &&
	       historical.historical_custody_indices == std::vector<size_t>{ 6 });
	unknown = cache;
	unknown[0].item_uid = 40000;
	assert(has(inspect(unknown, clocks, damaged),
		   economic_sql_cache_issue::historical_persisted_uid));
	// Original peek/hydrate permits an observed current owner clock0. Do not turn
	// optional0 into missing or fabricate a zero for optionalnull.
	damaged = persisted;
	auto zero = cache;
	auto zero_clocks = clocks;
	for (auto &owner : damaged.physical.source2.owners)
		owner.revision = 0;
	for (auto &native : damaged.physical.source2.items)
		native.owner_revision = 0;
	for (auto &entry : zero)
		entry.owner_revision = 0;
	for (auto &clock : zero_clocks)
		clock = 0;
	assert(inspect(zero, zero_clocks, damaged).findings.empty());
	std::vector<item_ownership_runtime_entry> missing(600, cache[0]);
	std::vector<std::optional<uint64_t>> missing_clocks(600, clocks[0]);
	for (size_t index = 0; index < missing.size(); ++index)
		missing[index].item_uid = 50000 + index;
	const auto full = inspect(missing, missing_clocks, persisted);
	assert(full.cache.size() == 600 && full.findings.size() == 600 &&
	       full.diagnostics.size() == 512 && full.diagnostics_truncated &&
	       full.issue_counts[size_t(economic_sql_cache_issue::missing_persisted_uid)] == 600);
	auto sentinel = []()
	{
		economic_sql_runtime_cache_correspondence report;
		report.historical_custody_indices = { 987 };
		return report;
	};
	for (size_t refusal = 0; refusal < 4; ++refusal)
	{
		auto output = sentinel();
		economic_sql_source_limits limits;
		auto observed = clocks;
		size_t details = 512;
		if (refusal == 0)
			observed.pop_back();
		if (refusal == 1)
			details = 0;
		if (refusal == 2)
			limits.maximum_rows = 5;
		if (refusal == 3)
			limits.maximum_cell_bytes = 1;
		assert(economic_sql_compare_runtime_cache(cache, observed, persisted, limits,
							  details, &output) !=
		       economic_accounting_error::ok);
		assert(output.historical_custody_indices == std::vector<size_t>{ 987 } &&
		       output.cache.empty() && output.findings.empty());
	}
	// Reuse original union allocator fault injection; every targeted allocation
	// must be reached and return capacity with unchanged staged output.
	auction_fault_seen = auction_fault_target = 0;
	auction_fault_enabled = true;
	economic_sql_runtime_cache_correspondence counted;
	const auto code =
		economic_sql_compare_runtime_cache(cache, clocks, persisted, {}, 512, &counted);
	auction_fault_enabled = false;
	assert(code == economic_accounting_error::ok && auction_fault_seen);
	const size_t allocations = auction_fault_seen;
	for (size_t at = 1; at <= allocations; ++at)
	{
		auto output = sentinel();
		auction_fault_seen = 0;
		auction_fault_target = at;
		auction_fault_enabled = true;
		const auto failure = economic_sql_compare_runtime_cache(cache, clocks, persisted,
									{}, 512, &output);
		auction_fault_enabled = false;
		assert(auction_fault_seen >= at && failure == economic_accounting_error::capacity &&
		       output.cache.empty() &&
		       output.historical_custody_indices == std::vector<size_t>{ 987 });
	}
}
