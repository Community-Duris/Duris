#include "economy/economic_sql_runtime_cache_correspondence.h"
#include <algorithm>
#include <new>
#include <tuple>
#include <utility>

namespace
{
struct cache_failure
{
	economic_accounting_error code;
};
void require_cache(bool value,
		   economic_accounting_error code = economic_accounting_error::corrupt_evidence)
{
	if (!value)
		throw cache_failure{ code };
}
using uid_index = std::pair<uint64_t, size_t>;
using owner_key = std::tuple<uint8_t, uint64_t, uint64_t>;
owner_key key(const item_owner_identity &owner)
{
	return { uint8_t(owner.type), owner.id, owner.context_id };
}
struct comparer
{
	std::span<const item_ownership_runtime_entry> cache;
	std::span<const std::optional<uint64_t>> clocks;
	const economic_sql_persisted_correspondence &source;
	const economic_sql_source_limits &limits;
	size_t diagnostic_limit;
	economic_sql_runtime_cache_correspondence result;
	std::vector<uid_index> cache_uids, sql_uids;
	std::vector<std::pair<owner_key, size_t>> owners;
	size_t bytes = 0;

	template <class T> void reserve(std::vector<T> &v, size_t count)
	{
		if (count <= v.capacity())
			return;
		const size_t delta = count - v.capacity();
		require_cache(bytes <= limits.maximum_cell_bytes &&
				      delta <= (limits.maximum_cell_bytes - bytes) / sizeof(T),
			      economic_accounting_error::capacity);
		bytes += delta * sizeof(T);
		v.reserve(count);
	}
	template <class T> void push(std::vector<T> &v, const T &value)
	{
		if (v.size() == v.capacity())
			reserve(v, v.capacity() ? v.capacity() * 2 : 1);
		v.push_back(value);
	}
	void finding(economic_sql_cache_issue issue, size_t c = SIZE_MAX, size_t s = SIZE_MAX)
	{
		push(result.findings, economic_sql_cache_finding{ issue, c, s });
		++result.issue_counts[size_t(issue)];
		if (result.diagnostics.size() < diagnostic_limit)
			push(result.diagnostics, economic_sql_cache_finding{ issue, c, s });
	}
	void cache_issue(size_t c, economic_sql_cache_issue issue, size_t s = SIZE_MAX)
	{
		const auto bit = uint32_t{ 1 } << uint8_t(issue);
		if (!(result.cache[c].issues & bit))
		{
			result.cache[c].issues |= bit;
			finding(issue, c, s);
		}
	}
	void sql_issue(size_t s, economic_sql_cache_issue issue)
	{
		const auto bit = uint32_t{ 1 } << uint8_t(issue);
		if (!(result.custody[s].issues & bit))
		{
			result.custody[s].issues |= bit;
			finding(issue, SIZE_MAX, s);
		}
	}
	auto range(const std::vector<uid_index> &values, uint64_t uid) const
	{
		return std::pair{
			std::lower_bound(values.begin(), values.end(), uid_index{ uid, 0 }),
			std::upper_bound(values.begin(), values.end(), uid_index{ uid, SIZE_MAX })
		};
	}
	bool provider_index(const economic_sql_persisted_match &match) const
	{
		switch (match.provider)
		{
		case economic_sql_persisted_provider::physical:
			return match.witness_index < source.physical.items.size();
		case economic_sql_persisted_provider::room:
			return match.witness_index < source.room.witnesses.size();
		case economic_sql_persisted_provider::auction:
			return match.witness_index < source.auction.nodes.size();
		case economic_sql_persisted_provider::collector:
			return match.witness_index < source.collector.listings.size();
		case economic_sql_persisted_provider::auction_legacy_identity:
			return match.witness_index < source.auction.legacy_identities.size();
		case economic_sql_persisted_provider::native_mobile_literal:
			return match.witness_index < source.native_mobile.items.size();
		case economic_sql_persisted_provider::room_creation:
			return source.creation_observed &&
			       match.witness_index < source.creation.witnesses.size();
		}
		return false;
	}
	void inspect()
	{
		const auto &items = source.physical.source2.items;
		const auto &sql_owners = source.physical.source2.owners;
		require_cache(source.custody.size() == items.size());
		reserve(result.cache, cache.size());
		result.cache.resize(cache.size());
		reserve(result.custody, items.size());
		result.custody.resize(items.size());
		reserve(cache_uids, cache.size());
		reserve(sql_uids, items.size());
		reserve(owners, sql_owners.size());
		for (size_t c = 0; c < cache.size(); ++c)
		{
			result.cache[c].cache_index = c;
			cache_uids.emplace_back(cache[c].item_uid, c);
			if (cache[c].state != item_custody_state::active)
				cache_issue(c, economic_sql_cache_issue::inactive_cache);
			if (!cache[c].item_uid || cache[c].item_uid == UINT64_MAX ||
			    !cache[c].root_item_uid || cache[c].root_item_uid == UINT64_MAX ||
			    cache[c].parent_item_uid == UINT64_MAX || !cache[c].item_revision ||
			    cache[c].item_revision == UINT64_MAX ||
			    !item_owner_identity_valid(cache[c].owner) || cache[c].vnum < 0)
				cache_issue(c, economic_sql_cache_issue::invalid_cache);
		}
		size_t match_count = 0;
		for (size_t s = 0; s < items.size(); ++s)
		{
			require_cache(source.custody[s].matches.size() <=
					      limits.maximum_cells - match_count,
				      economic_accounting_error::capacity);
			match_count += source.custody[s].matches.size();
			require_cache(source.custody[s].custody_index == s);
			for (const auto &match : source.custody[s].matches)
				require_cache(provider_index(match));
			result.custody[s].custody_index = s;
			result.custody[s].current_provider_count = source.custody[s].matches.size();
			sql_uids.emplace_back(items[s].item.uid, s);
			if (items[s].item.position.state != item_custody_state::active)
			{
				push(result.historical_custody_indices, s);
				if (!source.custody[s].matches.empty())
					sql_issue(
						s,
						economic_sql_cache_issue::historical_persisted_uid);
			}
			else if (source.custody[s].matches.empty())
				sql_issue(s, economic_sql_cache_issue::missing_provider);
			else if (source.custody[s].matches.size() != 1)
				sql_issue(s, economic_sql_cache_issue::multiple_providers);
		}
		for (size_t o = 0; o < sql_owners.size(); ++o)
			owners.emplace_back(key(sql_owners[o].owner), o);
		std::sort(cache_uids.begin(), cache_uids.end());
		std::sort(sql_uids.begin(), sql_uids.end());
		std::sort(owners.begin(), owners.end());
		// UID groups are processed once, preserving every duplicate without a
		// duplicate-cache times duplicate-custody Cartesian product.
		for (size_t first = 0; first < cache_uids.size();)
		{
			size_t last = first + 1;
			while (last < cache_uids.size() &&
			       cache_uids[last].first == cache_uids[first].first)
				++last;
			if (last - first > 1)
				for (size_t n = first; n < last; ++n)
					cache_issue(cache_uids[n].second,
						    economic_sql_cache_issue::duplicate_cache_uid);
			first = last;
		}
		for (size_t first = 0; first < sql_uids.size();)
		{
			size_t last = first + 1;
			while (last < sql_uids.size() &&
			       sql_uids[last].first == sql_uids[first].first)
				++last;
			if (last - first > 1)
				for (size_t n = first; n < last; ++n)
					sql_issue(
						sql_uids[n].second,
						economic_sql_cache_issue::duplicate_persisted_uid);
			// One observed cache cardinality per UID, retained on every SQL
			// row even when duplicate custody makes matching ambiguous.
			const auto [cache_first, cache_last] =
				range(cache_uids, sql_uids[first].first);
			const auto active_count = static_cast<size_t>(
				std::count_if(cache_first, cache_last,
					      [&](const uid_index &value) {
						      return cache[value.second].state ==
							     item_custody_state::active;
					      }));
			for (size_t n = first; n < last; ++n)
				result.custody[sql_uids[n].second].active_cache_count =
					active_count;
			first = last;
		}
		for (size_t c = 0; c < cache.size(); ++c)
		{
			const auto &entry = cache[c];
			if (entry.state != item_custody_state::active)
				continue;
			if (!clocks[c])
				cache_issue(c,
					    economic_sql_cache_issue::missing_runtime_owner_clock);
			else if (entry.owner_revision > *clocks[c])
				cache_issue(c, economic_sql_cache_issue::future_entry_clock);
			const auto identity = key(entry.owner);
			auto owner_first = std::lower_bound(owners.begin(), owners.end(),
							    std::pair{ identity, size_t{ 0 } });
			auto owner_last = std::upper_bound(owners.begin(), owners.end(),
							   std::pair{ identity, SIZE_MAX });
			if (owner_first == owner_last)
				cache_issue(c, economic_sql_cache_issue::missing_sql_owner_clock);
			else if (owner_last - owner_first != 1)
				cache_issue(c, economic_sql_cache_issue::duplicate_sql_owner_clock);
			else
			{
				result.cache[c].sql_owner_index = owner_first->second;
				const uint64_t sql_clock = sql_owners[owner_first->second].revision;
				if (clocks[c] && *clocks[c] != sql_clock)
					cache_issue(c, economic_sql_cache_issue::
							       current_owner_clock_mismatch);
				if (entry.owner_revision > sql_clock)
					cache_issue(c,
						    economic_sql_cache_issue::future_entry_clock);
			}
			auto [first, last] = range(sql_uids, entry.item_uid);
			if (first == last)
			{
				cache_issue(c, economic_sql_cache_issue::missing_persisted_uid);
				continue;
			}
			if (last - first != 1)
			{
				cache_issue(c, economic_sql_cache_issue::duplicate_persisted_uid);
				continue;
			}
			const size_t s = first->second;
			result.cache[c].custody_index = s;
			const auto &native = items[s];
			const auto &position = native.item.position;
			if (position.state != item_custody_state::active)
				cache_issue(c, economic_sql_cache_issue::historical_persisted_uid,
					    s);
			if (entry.item_uid != native.item.uid ||
			    entry.root_item_uid != position.root_uid ||
			    entry.parent_item_uid != position.parent_uid ||
			    !item_owner_identity_equal(entry.owner, position.owner) ||
			    entry.state != position.state || entry.vnum != native.vnum ||
			    entry.item_revision != position.revision)
				cache_issue(c, economic_sql_cache_issue::fields_mismatch, s);
			if (source.custody[s].matches.empty())
				cache_issue(c, economic_sql_cache_issue::missing_provider, s);
			else if (source.custody[s].matches.size() != 1)
				cache_issue(c, economic_sql_cache_issue::multiple_providers, s);
			if (!native.owner_revision)
				cache_issue(c, economic_sql_cache_issue::missing_sql_owner_clock,
					    s);
			else if (result.cache[c].sql_owner_index &&
				 *native.owner_revision !=
					 sql_owners[*result.cache[c].sql_owner_index].revision)
				cache_issue(c,
					    economic_sql_cache_issue::current_owner_clock_mismatch,
					    s);
			result.cache[c].exact_current_correspondence = !result.cache[c].issues &&
								       !result.custody[s].issues;
		}
		result.diagnostics_truncated = result.findings.size() > result.diagnostics.size();
	}
};
}
economic_accounting_error economic_sql_compare_runtime_cache(
	std::span<const item_ownership_runtime_entry> cache,
	std::span<const std::optional<uint64_t>> current_runtime_owner_revisions,
	const economic_sql_persisted_correspondence &source,
	const economic_sql_source_limits &limits, size_t diagnostic_limit,
	economic_sql_runtime_cache_correspondence *output) noexcept
{
	try
	{
		const economic_sql_source_limits hard;
		require_cache(output && diagnostic_limit && diagnostic_limit <= 512 &&
			      cache.size() == current_runtime_owner_revisions.size());
		require_cache(limits.maximum_rows && limits.maximum_rows <= hard.maximum_rows &&
			      limits.maximum_cell_bytes &&
			      limits.maximum_cell_bytes <= hard.maximum_cell_bytes &&
			      limits.maximum_cells && limits.maximum_cells <= hard.maximum_cells &&
			      limits.maximum_single_cell_bytes &&
			      limits.maximum_single_cell_bytes <= hard.maximum_single_cell_bytes);
		require_cache(cache.size() <= limits.maximum_rows &&
				      source.physical.source2.items.size() <= limits.maximum_rows &&
				      source.physical.source2.owners.size() <= limits.maximum_rows,
			      economic_accounting_error::capacity);
		comparer value{ cache,
				current_runtime_owner_revisions,
				source,
				limits,
				diagnostic_limit,
				{},
				{},
				{},
				{},
				0 };
		value.inspect();
		*output = std::move(value.result);
		return economic_accounting_error::ok;
	}
	catch (const cache_failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	catch (...)
	{
		return economic_accounting_error::corrupt_evidence;
	}
}
