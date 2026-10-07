// Disposable original native codec/query oracle. Never use an existing root.
// Compile the original repository here to observe its complete private catalog;
// the independent reader does not include this file or any native provider.
#include "flatfile/flatfile_auction_repository.c"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;
static uint64_t plan5_number(const std::vector<uint8_t> &bytes, size_t offset, size_t width)
{
	assert(offset <= bytes.size() && width <= bytes.size() - offset);
	uint64_t result = 0;
	for (size_t i = 0; i < width; ++i)
		result |= uint64_t(bytes[offset + i]) << (8 * i);
	return result;
}
static std::vector<uint8_t> plan5_read(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary | std::ios::ate);
	assert(input);
	const auto size = input.tellg();
	assert(size >= 0 && size <= 8 * 1024 * 1024);
	std::vector<uint8_t> bytes(static_cast<size_t>(size));
	input.seekg(0);
	assert(input.read(reinterpret_cast<char *>(bytes.data()), size));
	return bytes;
}
// Only this disposable fixture bridges original common-record decoding to the
// original native receipt codec. The operator never imports this helper.
static flatfile_accounting_record plan5_creator(const std::string &root)
{
	for (const auto &entry : fs::directory_iterator(fs::path(root) / "economic-evidence"))
	{
		const auto name = entry.path().filename().string();
		if (!name.starts_with("bucket-") || !name.ends_with(".eai"))
			continue;
		const auto index = plan5_read(entry.path());
		const auto count = plan5_number(index, 68, 4);
		for (size_t i = 0; i < count; ++i)
		{
			const size_t at = 80 + i * 64;
			const auto segment = plan5_number(index, at + 48, 4),
				   offset = plan5_number(index, at + 52, 4),
				   size = plan5_number(index, at + 56, 4);
			const auto bytes = plan5_read(
				entry.path().parent_path() /
				(name.substr(0, 9) + "-" + std::to_string(segment) + ".eas"));
			assert(80 + offset <= bytes.size() && size <= bytes.size() - 80 - offset);
			flatfile_accounting_record record;
			assert(flatfile_accounting_record_decode(
				       std::span(bytes).subspan(80 + offset, size), &record) ==
			       flatfile_accounting_status::ok);
			if (record.command.type == critical_command_type::auction)
				return record;
		}
	}
	assert(false);
	return {};
}
static void plan5_seed_attribution(const std::string &root, const std::string &mode)
{
	flatfile_authority_lock lock;
	std::string error;
	assert(lock.acquire(root, &error) &&
	       !fs::exists(fs::path(root) / "domains/auction_catalog"));
	const auto retained = plan5_creator(root);
	assert(retained.result_code == 0);
	auction_command_payload payload = {};
	auction_operation operation;
	operation.operation_id = retained.command.operation_id;
	assert(auction_command_decode_payload(retained.command, &payload));
	assert(auction_command_decode_result(retained.result.data(), retained.result.size(),
					     &operation.result));
	std::vector<uint8_t> command;
	assert(critical_command_encode(retained.command, &command) ==
	       critical_command_codec_result::ok);
	SHA256(command.data(), command.size(), operation.command_digest.data());
	const bool bid = payload.action == auction_action::bid;
	const bool sale = operation.result.event_type == auction_event_type::sold;
	const bool born = mode.find("born") != std::string::npos;
	auction_catalog catalog;
	catalog.revision = 10;
	auction_listing listing;
	listing.id = payload.auction_id;
	listing.seller_pid = operation.result.seller_pid;
	listing.winner_pid = operation.result.winner_pid;
	listing.status = operation.result.status;
	listing.current_price = operation.result.final_price;
	listing.revision = operation.result.auction_revision;
	listing.end_time = 100;
	listing.seller_account = "native-fixture";
	listing.object_blob = { 11, 22, 33 };
	listing.items.push_back(
		{ 42, 3, 10, sale ? listing.winner_pid : listing.seller_pid, false });
	catalog.listings.push_back(listing);
	catalog.operations.push_back(operation);
	auction_claim_source_catalog sources;
	critical_operation_id lineage = {};
	lineage.bytes[0] = 1;
	if (bid)
	{
		catalog.money.push_back({ 11, 0, 1 });
		const int64_t proceeds = sale ? operation.result.final_price -
							 operation.result.final_price *
								 payload.closing_fee_basis_points /
								 10000 :
						0;
		catalog.money.push_back({ 22, proceeds, sale ? 2u : 1u });
		catalog.money.push_back({ INT32_MAX, 60, 4 });
		assert(add_source(&sources, retained.command,
				  { lineage, economic_account_kind::pending_claim, 5, 0 },
				  INT32_MAX, 1, 60));
		if (proceeds)
			assert(add_source(&sources, retained.command,
					  { lineage, economic_account_kind::pending_claim, 8, 0 },
					  22, 2, proceeds));
	}
	else
	{
		const int64_t proceeds = sale ? operation.result.final_price -
							 operation.result.final_price *
								 payload.closing_fee_basis_points /
								 10000 :
						0;
		catalog.money.push_back({ INT32_MAX, proceeds, sale ? born ? 1u : 4u : 3u });
		if (proceeds)
			assert(add_source(&sources, retained.command,
					  { lineage, economic_account_kind::pending_claim,
					    born ? 7u : 5u, 0 },
					  INT32_MAX, 2, proceeds));
	}
	std::vector<uint8_t> encoded;
	assert(encode_catalog(catalog, &encoded));
	assert(flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, &error));
	// Model an initialized empty source cut when this recipe grants no credits.
	// This is codec/reader evidence, not proof that a live producer creates it.
	if (!sources.revision)
		sources.revision = 1;
	assert(encode_sources(sources, &encoded));
	assert(flatfile_atomic_write(domains_directory(root), source_filename, encoded, &error));
}
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
	if (mode.starts_with("seed-attribution-"))
		plan5_seed_attribution(root, mode);
	else if (mode == "seed")
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
