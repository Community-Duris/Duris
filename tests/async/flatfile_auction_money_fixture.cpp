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
static std::vector<flatfile_accounting_record> plan5_records(const std::string &root)
{
	std::vector<flatfile_accounting_record> records;
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
				records.push_back(std::move(record));
		}
	}
	return records;
}
static std::vector<std::pair<flatfile_accounting_record, economic_account_effect>>
plan5_claim_timeline(const std::string &root)
{
	std::vector<std::pair<flatfile_accounting_record, economic_account_effect>> timeline;
	for (const auto &record : plan5_records(root))
	{
		economic_accounting_plan plan;
		assert(economic_plan_decode(record.plan, &plan) == economic_accounting_error::ok);
		for (const auto &effect : plan.accounts)
			if (effect.key.kind == economic_account_kind::pending_claim &&
			    effect.key.authority_id == 5)
				timeline.emplace_back(record, effect);
	}
	std::sort(timeline.begin(), timeline.end(),
		  [](const auto &a, const auto &b)
		  { return a.second.before_revision < b.second.before_revision; });
	return timeline;
}
static auction_operation plan5_receipt(const flatfile_accounting_record &record)
{
	auction_operation result;
	result.operation_id = record.command.operation_id;
	assert(auction_command_decode_result(record.result.data(), record.result.size(),
					     &result.result));
	std::vector<uint8_t> encoded;
	assert(critical_command_encode(record.command, &encoded) ==
	       critical_command_codec_result::ok);
	SHA256(encoded.data(), encoded.size(), result.command_digest.data());
	return result;
}
static void plan5_seed_chain(const std::string &root, bool oversize)
{
	flatfile_authority_lock lock;
	std::string error;
	assert(lock.acquire(root, &error) &&
	       !fs::exists(fs::path(root) / "domains/auction_catalog"));
	auction_catalog catalog;
	catalog.revision = 10;
	auction_claim_source_catalog sources;
	int64_t balance = 0;
	uint64_t revision = 3;
	for (const auto &[record, effect] : plan5_claim_timeline(root))
	{
		assert(record.result_code == 0 && effect.before_revision == revision &&
		       effect.before[0] == balance);
		const int64_t delta = effect.after[0] - effect.before[0];
		if (delta > 0)
			assert(add_source(&sources, record.command, effect.key, INT32_MAX, 2,
					  delta));
		else
		{
			const bool accepted = consume_whole_claim_sources(
				&sources, record.command, effect.key, INT32_MAX, balance, -delta);
			if (oversize && record.command.operation_id.bytes[0] == 8)
			{
				assert(!accepted && sources.rows.size() == 2 &&
				       sources.rows[1].amount == uint64_t(-delta));
				sources.rows[1].consumed_by = record.command.operation_id;
				++sources.revision;
			}
			else
				assert(accepted);
		}
		balance = effect.after[0];
		revision = effect.after_revision;
		catalog.operations.push_back(plan5_receipt(record));
		const auto &result = catalog.operations.back().result;
		if (!result.auction_id)
			continue;
		auction_listing listing;
		listing.id = result.auction_id;
		listing.seller_pid = result.seller_pid;
		listing.winner_pid = result.winner_pid;
		listing.status = result.status;
		listing.current_price = result.final_price;
		listing.revision = result.auction_revision;
		listing.end_time = 100;
		listing.seller_account = "native-fixture";
		listing.object_blob = { 11, 22, 33 };
		listing.items.push_back({ uint64_t(result.auction_id == UINT32_MAX ? 42 : 43), 3,
					  10, result.winner_pid, false });
		catalog.listings.push_back(listing);
	}
	std::sort(catalog.listings.begin(), catalog.listings.end(),
		  [](const auto &a, const auto &b) { return a.id < b.id; });
	std::sort(catalog.operations.begin(), catalog.operations.end(),
		  [](const auto &a, const auto &b)
		  { return a.operation_id.bytes < b.operation_id.bytes; });
	catalog.money.push_back({ INT32_MAX, balance, revision });
	std::vector<uint8_t> encoded;
	assert(encode_catalog(catalog, &encoded));
	assert(flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, &error));
	assert(encode_sources(sources, &encoded));
	assert(flatfile_atomic_write(domains_directory(root), source_filename, encoded, &error));
}
static void plan5_seed_attribution(const std::string &root, const std::string &mode)
{
	flatfile_authority_lock lock;
	std::string error;
	assert(lock.acquire(root, &error) &&
	       !fs::exists(fs::path(root) / "domains/auction_catalog"));
	const auto records = plan5_records(root);
	const auto found = std::find_if(records.begin(), records.end(),
					[](const auto &row)
					{ return row.command.operation_id.bytes[0] == 6; });
	assert(found != records.end());
	const auto &retained = *found;
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
	if (mode.find("consumption-money") != std::string::npos)
	{
		const auto consumer = std::find_if(
			records.begin(), records.end(),
			[](const auto &row) { return row.command.operation_id.bytes[0] == 8; });
		assert(consumer != records.end() && consumer->result_code == 0);
		assert(consume_whole_claim_sources(&sources, consumer->command,
						   { lineage, economic_account_kind::pending_claim,
						     5, 0 },
						   INT32_MAX, 60, 60));
		auto *claim = find_money(&catalog, INT32_MAX);
		assert(claim && claim->amount == 60 && claim->revision == 4);
		claim->amount = 0;
		++claim->revision;
		auction_operation receipt;
		receipt.operation_id = consumer->command.operation_id;
		assert(auction_command_decode_result(consumer->result.data(),
						     consumer->result.size(), &receipt.result));
		command.clear();
		assert(critical_command_encode(consumer->command, &command) ==
		       critical_command_codec_result::ok);
		SHA256(command.data(), command.size(), receipt.command_digest.data());
		catalog.operations.push_back(receipt);
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
	if (mode.starts_with("seed-attribution-consumption-chain-"))
		plan5_seed_chain(root, mode.ends_with("oversize"));
	else if (mode.starts_with("seed-attribution-"))
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
		assert(mode == "probe" || mode == "public-query" || mode == "source-balance" ||
		       mode == "consumer-proof");
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
	if (mode == "consumer-proof")
	{
		const auto records = plan5_records(root);
		bool frozen_match = true, whole_match = true;
		size_t cashouts = 0, rows = 0;
		auction_claim_source_catalog expected_sources;
		for (const auto &[record, effect] : plan5_claim_timeline(root))
		{
			const auto delta = effect.after[0] - effect.before[0];
			if (delta > 0)
				whole_match = add_source(&expected_sources, record.command,
							 effect.key, INT32_MAX, 2, delta) &&
					      whole_match;
			else
				whole_match = consume_whole_claim_sources(
						      &expected_sources, record.command, effect.key,
						      INT32_MAX, effect.before[0], -delta) &&
					      whole_match;
		}
		whole_match = whole_match && expected_sources.rows.size() == sources.rows.size();
		if (whole_match)
			for (size_t i = 0; i < sources.rows.size(); ++i)
				whole_match = critical_operation_id_equal(
						      expected_sources.rows[i].consumed_by,
						      sources.rows[i].consumed_by) &&
					      whole_match;
		for (const auto &record : records)
		{
			auction_command_payload payload;
			assert(auction_command_decode_payload(record.command, &payload));
			if (payload.action != auction_action::claim_money)
				continue;
			++cashouts;
			const auto *row = &record;
			economic_frozen_intent intent;
			assert(economic_intent_decode(row->command.accounting_intent, &intent) ==
			       economic_accounting_error::ok);
			const auto &wire = intent.admission.facts;
			assert(wire.size() == 80);
			const auto lineage = intent.admission.metadata.lineage;
			const economic_account_key wallet{ lineage, economic_account_kind::wallet,
							   plan5_number(wire, 0, 8), 0 };
			const economic_account_key bank{ lineage, economic_account_kind::bank,
							 plan5_number(wire, 8, 8), 1 };
			const economic_account_key claim_account{
				lineage, economic_account_kind::pending_claim,
				plan5_number(wire, 16, 8), 0
			};
			auction_money_claim_state claim;
			claim.beneficiary_pid = plan5_number(wire, 24, 4);
			claim.money = plan5_number(wire, 28, 8);
			claim.revision = plan5_number(wire, 36, 8);
			for (const auto &source : sources.rows)
				if (critical_operation_id_equal(source.consumed_by,
								row->command.operation_id))
					claim.sources.push_back({ source.operation, source.slot,
								  source.beneficiary_pid,
								  source.claim_mapping_id,
								  source.amount });
			critical_command projected = row->command;
			projected.schema_version = 1;
			projected.accounting_intent.clear();
			projected.publication_required = false;
			std::vector<uint8_t> expected;
			const auto status = auction_money_claim_accounting_intent(
				projected, intent.admission.metadata.epoch, wallet, bank,
				claim_account, claim, &expected);
			frozen_match = frozen_match && status == economic_accounting_error::ok &&
				       expected == row->command.accounting_intent;
			rows += claim.sources.size();
		}
		assert(cashouts);
		std::cout << "{\"native_frozen_consumer_sources_match\":"
			  << (frozen_match ? "true" : "false")
			  << ",\"native_whole_consumption_match\":"
			  << (whole_match ? "true" : "false") << ",\"cashouts\":" << cashouts
			  << ",\"observed_consumed_rows\":" << rows << "}\n";
	}
	else if (mode == "source-balance")
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
