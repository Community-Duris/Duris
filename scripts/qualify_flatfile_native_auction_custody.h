// Independent retained auction roots/claims and current custody; no provider calls.
#ifndef DURIS_QUALIFY_FLATFILE_NATIVE_AUCTION_CUSTODY_H
#define DURIS_QUALIFY_FLATFILE_NATIVE_AUCTION_CUSTODY_H

#include "qualify_flatfile_native_auction.h"
#include "qualify_flatfile_native_custody.h"

namespace restore_native_auction_custody
{
using namespace restore_native_custody;
struct finding
{
	const char *code;
	uint64_t uid;
};
struct result
{
	bool auction_present = false, custody_present = false;
	uint32_t auction_version = 0;
	uint64_t auction_revision = 0;
	size_t listings = 0, retained_roots = 0, unclaimed_roots = 0, claimed_roots = 0,
	       compared_roots = 0, admitted_claimed_roots = 0, custody_auction_items = 0,
	       other_custody_items = 0, equipment_fields_absent = 0, coin_payloads_uncompared = 0,
	       serialized_templates_uncompared = 0, finding_count = 0;
	std::map<std::string, size_t> finding_counts;
	std::vector<finding> findings;
	bool valid() const { return !finding_count; }
	bool verified() const
	{
		return auction_present && custody_present && valid() && !equipment_fields_absent;
	}
};
inline result audit(const std::filesystem::path &root, audit_budget &budget,
		    size_t detail_limit = 100)
{
	need(detail_limit <= 100);
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	lock.no_pending_player_domains();
	const auto domains = root / "domains";
	const auto read = [&](const char *name, size_t maximum)
	{
		struct stat info = {};
		if (lstat((domains / name).c_str(), &info) != 0)
		{
			need(errno == ENOENT);
			return bytes{};
		}
		need(lock.locked());
		auto encoded = file_bytes(domains, name, maximum);
		need(!encoded.empty());
		return encoded;
	};
	const auto custody_bytes = read("item_ownership", catalog_limit);
	const auto auction_bytes = read("auction_catalog", restore_native_auction::catalog_limit);
	result output;
	output.custody_present = !custody_bytes.empty();
	output.auction_present = !auction_bytes.empty();
	restore_native_custody::catalog custody{};
	if (output.custody_present)
		custody = decode_catalog(custody_bytes);
	const auto issue = [&](const char *code, uint64_t uid = 0)
	{
		++output.finding_count;
		++output.finding_counts[code];
		if (output.findings.size() < detail_limit)
			output.findings.push_back({ code, uid });
	};
	std::map<uint64_t, const item *> owned;
	for (const auto &row : custody.items)
	{
		audit_checkpoint();
		owned.emplace(row.uid, &row);
		if (row.state == 2)
			continue;
		if (row.location.type == 6)
			++output.custody_auction_items;
		else
			++output.other_custody_items;
	}
	std::set<uint64_t> seen;
	if (output.auction_present)
	{
		const auto auctions = restore_native_auction::decode_catalog(
			auction_bytes, {},
			[&](const restore_native_auction::listing &listing)
			{
				++output.serialized_templates_uncompared;
				for (const auto &literal : listing.items)
				{
					audit_checkpoint();
					++output.retained_roots;
					const auto claimant = listing.status == 3 ||
									      !listing.winner ?
								      listing.seller :
								      listing.winner;
					if (listing.status == 1 ?
						    literal.claim_pid || literal.claimed :
						    literal.claim_pid != claimant)
						issue("auction_claim_state_invalid", literal.uid);
					const auto found = owned.find(literal.uid);
					if (literal.claimed)
					{
						++output.claimed_roots;
						if (found == owned.end())
							issue("auction_claimed_uid_unadmitted",
							      literal.uid);
						else
							++output.admitted_claimed_roots;
						// A completed claim is history, never current ownership.
						continue;
					}
					++output.unclaimed_roots;
					if (!seen.insert(literal.uid).second)
						issue("auction_unclaimed_uid_duplicate",
						      literal.uid);
					if (found == owned.end())
					{
						issue("auction_uid_unadmitted", literal.uid);
						continue;
					}
					++output.compared_roots;
					const auto &row = *found->second;
					if (row.state != 1)
						issue("auction_uid_not_active", row.uid);
					if (row.location != owner{ 6, listing.id, 0 })
						issue("auction_owner_mismatch", row.uid);
					if (row.root != row.uid || row.parent)
						issue("auction_item_topology_mismatch", row.uid);
					if (row.revision != literal.revision)
						issue("auction_item_revision_mismatch", row.uid);
					if (row.vnum != literal.vnum)
						issue("auction_item_vnum_mismatch", row.uid);
					if (custody.version < 5)
						++output.equipment_fields_absent;
					else if (row.equipment)
						issue("auction_item_equipment_mismatch", row.uid);
					if (!row.coin_payload.empty())
						++output.coin_payloads_uncompared;
				}
			});
		output.auction_version = auctions.version;
		output.auction_revision = auctions.revision;
		output.listings = auctions.listings;
	}
	if (output.retained_roots && !output.custody_present)
		issue("auction_custody_catalog_missing");
	if (output.custody_auction_items && !output.auction_present)
		issue("custody_auction_catalog_missing");
	for (const auto &row : custody.items)
	{
		audit_checkpoint();
		if (row.state != 2 && row.location.type == 6 && !seen.contains(row.uid))
			issue("auction_uid_missing_unclaimed_root", row.uid);
	}
	lock.no_pending_player_domains();
	lock.finish();
	budget.checkpoint();
	return output;
}
} // namespace restore_native_auction_custody
#endif
