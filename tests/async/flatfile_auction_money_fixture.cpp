// Disposable original native codec/query oracle. Never use an existing root.
// Compile the original repository here to observe its complete private catalog;
// the independent reader does not include this file or any native provider.
#include "flatfile/flatfile_auction_repository.c"
#include <cassert>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
static void plan5_output(const auction_catalog &catalog,
			 const auction_claim_source_catalog &sources)
{
	std::cout << "{\"catalog_revision\":" << catalog.revision
		  << ",\"listings\":" << catalog.listings.size()
		  << ",\"operations\":" << catalog.operations.size()
		  << ",\"source_revision\":" << sources.revision
		  << ",\"source_rows\":" << sources.rows.size() << ",\"holdings\":[";
	bool comma = false;
	for (const auto &listing : catalog.listings)
		if (listing.status == auction_status_open)
		{
			std::cout << (comma ? "," : "") << "[4," << listing.id << ','
				  << (listing.winner_pid ? listing.current_price : 0) << ','
				  << listing.revision << ']';
			comma = true;
		}
	for (const auto &money : catalog.money)
	{
		std::cout << (comma ? "," : "") << "[5," << money.pid << ',' << money.amount << ','
			  << money.revision << ']';
		comma = true;
	}
	std::cout << "]}\n";
}
int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string mode = argv[1], root = argv[2];
	assert(fs::path(root).is_absolute() && fs::is_directory(root));
	std::string error;
	if (mode == "seed")
	{
		assert(fs::is_empty(root) && fs::create_directory(fs::path(root) / "domains"));
		fs::permissions(fs::path(root) / "domains", fs::perms::owner_all);
		flatfile_authority_lock lock;
		assert(lock.acquire(root, &error));
		auction_catalog catalog;
		catalog.revision = 10;
		auction_listing listing;
		listing.id = UINT32_MAX;
		listing.seller_pid = 11;
		listing.winner_pid = 22;
		listing.status = auction_status_open;
		listing.current_price = 70;
		listing.buy_price = 150;
		listing.revision = 3;
		listing.end_time = 100;
		listing.seller_account = "native-fixture";
		listing.seller_name = "Seller";
		listing.winner_name = "Buyer";
		listing.object_short = "fixture-object";
		listing.id_keywords = "fixture";
		listing.object_info = "modeled native catalog, not a live producer proof";
		listing.object_blob = { 11, 22, 33 };
		listing.items.push_back({ 42, 3, 10, INT32_MAX, false });
		catalog.listings.push_back(listing);
		catalog.money.push_back({ INT32_MAX, 60, 4 });
		auction_operation receipt;
		receipt.operation_id.bytes[0] = 5;
		receipt.command_digest.fill(7);
		receipt.result.action = auction_action::bid;
		receipt.result.event_type = auction_event_type::bid_placed;
		receipt.result.auction_id = listing.id;
		receipt.result.status = auction_status_open;
		receipt.result.seller_pid = listing.seller_pid;
		receipt.result.winner_pid = listing.winner_pid;
		receipt.result.final_price = listing.current_price;
		receipt.result.auction_revision = listing.revision;
		receipt.result.item_count = 1;
		receipt.result.item_uids[0] = 42;
		receipt.result.item_revisions[0] = 3;
		catalog.operations.push_back(receipt);
		auction_claim_source_catalog sources;
		sources.revision = 8;
		auction_claim_source_row row;
		row.operation.bytes[0] = 6;
		row.slot = 1;
		row.lineage.bytes[0] = 1;
		row.beneficiary_pid = INT32_MAX;
		row.claim_mapping_id = 5;
		row.amount = 60;
		sources.rows.push_back(row);
		std::vector<uint8_t> bytes;
		assert(encode_catalog(catalog, &bytes));
		assert(flatfile_atomic_write(domains_directory(root), catalog_filename, bytes,
					     &error));
		assert(encode_sources(sources, &bytes));
		assert(flatfile_atomic_write(domains_directory(root), source_filename, bytes,
					     &error));
		assert(!fs::exists(fs::path(root) / "economic-evidence"));
	}
	else
		assert(mode == "probe" || mode == "public-query" || mode == "source-balance");
	auction_catalog catalog;
	auction_claim_source_catalog sources;
	if (mode == "public-query")
	{
		if (query_catalog(root, &catalog, &error) != flatfile_auction_query_result::ok)
		{
			std::cerr << "native_auction_decode_refused\n";
			return 1;
		}
	}
	else if (load_catalog(root, &catalog, &error) != flatfile_read_result::ok)
	{
		std::cerr << "native_auction_decode_refused\n";
		return 1;
	}
	if (load_sources(root, &sources, &error) != flatfile_read_result::ok)
	{
		std::cerr << "native_auction_decode_refused\n";
		return 1;
	}
	if (mode == "source-balance")
	{
		const auto *pickup = find_money(&catalog, INT32_MAX);
		assert(pickup);
		const economic_account_key account = { []
						       {
							       critical_operation_id value = {};
							       value.bytes[0] = 1;
							       return value;
						       }(),
						       economic_account_kind::pending_claim, 5, 0 };
		std::cout << "{\"claim_amount\":" << pickup->amount
			  << ",\"source_balance_matches\":"
			  << (claim_source_balance_matches(sources, account, INT32_MAX,
							   pickup->amount) ?
				      "true" :
				      "false")
			  << "}\n";
	}
	else
		plan5_output(catalog, sources);
}
