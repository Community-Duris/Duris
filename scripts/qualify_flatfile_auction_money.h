// Independent current auction escrow/claim comparison. Never mutates findings.
#ifndef DURIS_QUALIFY_FLATFILE_AUCTION_MONEY_H
#define DURIS_QUALIFY_FLATFILE_AUCTION_MONEY_H

#include "qualify_flatfile_current_money.h"
#include "qualify_flatfile_native_auction.h"

namespace restore_auction_money
{
using namespace restore_current_money;
struct result : restore_current_money::result
{
	bool catalog_present = false, sources_present = false;
	uint64_t catalog_revision = 0, source_revision = 0;
	size_t listings = 0, operations = 0, source_rows = 0;
	size_t native_escrows = 0, native_claims = 0;
};
struct observers
{
	std::function<void(const account_key &, const restore_economic_authority::mapping &)>
		mapping;
	std::function<void(const restore_native_auction::source_row &)> source;
	std::function<void(const restore_native_auction::money &)> holding;
	std::function<void(ledger &, result &)> finish;
};
inline result audit(const std::filesystem::path &root, audit_budget &budget,
		    const observers &observe = {})
{
	result output;
	ledger current(root, budget, 4, 5, output, observe.mapping);
	const auto domains = root / "domains";
	std::set<std::string> present;
	if (std::filesystem::exists(domains))
		for (const auto &entry : std::filesystem::directory_iterator(domains))
		{
			audit_directory_entry();
			const auto leaf = entry.path().filename().string();
			if (leaf == "auction_catalog" || leaf == "auction_claim_sources")
				present.insert(leaf);
		}
	if (present.contains("auction_claim_sources"))
	{
		need(current.locked());
		const auto sources = restore_native_auction::decode_sources(
			file_bytes(domains, "auction_claim_sources",
				   restore_native_auction::source_limit),
			observe.source);
		output.sources_present = true;
		output.source_revision = sources.revision;
		output.source_rows = sources.rows;
	}
	if (present.contains("auction_catalog"))
	{
		need(current.locked());
		const auto catalog = restore_native_auction::decode_catalog(file_bytes(
			domains, "auction_catalog", restore_native_auction::catalog_limit));
		output.catalog_present = true;
		output.catalog_revision = catalog.revision;
		output.listings = catalog.listings;
		output.operations = catalog.operations;
		for (const auto &holding : catalog.holdings)
		{
			if (output.native_escrows + output.native_claims == maximum_accounts)
				throw account_budget_refused();
			if (holding.kind == 4)
				++output.native_escrows;
			else
				++output.native_claims;
			current.holding(native_key(holding.kind, 0, holding.id),
					{ holding.amount, 0, 0, 0 }, holding.revision, holding.kind,
					holding.id);
			if (observe.holding)
				observe.holding(holding);
		}
	}
	current.finish(
		[&](ledger &locked)
		{
			if (observe.finish)
				observe.finish(locked, output);
		});
	return output;
}
} // namespace restore_auction_money
#endif
