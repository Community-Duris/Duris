#include "economy/economic_sql_source_normalize.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <limits>
#include <map>
#include <new>
#include <tuple>

namespace
{
using issue = economic_sql_normalization_issue;
using disposition = economic_sql_holding_disposition;
using kind = economic_sql_holding_kind;
using cell = std::optional<std::string>;
struct failure
{
	economic_accounting_error code;
};
void require(bool valid)
{
	if (!valid)
		throw failure{ economic_accounting_error::corrupt_evidence };
}
template <class T> T integer(const cell &value)
{
	require(value && !value->empty());
	T number = 0;
	const auto parsed = std::from_chars(value->data(), value->data() + value->size(), number);
	require(parsed.ec == std::errc{} && parsed.ptr == value->data() + value->size() &&
		std::to_string(number) == *value);
	return number;
}
struct consumer
{
	const economic_sql_source_snapshot &input;
	size_t limit;
	economic_sql_normalized_sources report;
	size_t source(const char *name) const
	{
		auto found = std::find_if(input.tables.begin(), input.tables.end(),
					  [&](const auto &table) { return table.name == name; });
		require(found != input.tables.end());
		return found - input.tables.begin();
	}
	economic_sql_source_reference reference(size_t table, size_t row) const
	{
		return { table, row,
			 row == SIZE_MAX ? input.tables[table].content_digest :
					   input.tables[table].rows[row].digest };
	}
	void observe(issue code, const economic_sql_source_reference &where)
	{
		++report.issue_counts[static_cast<size_t>(code)];
		++report.diagnostic_count;
		if (report.diagnostics.size() < limit)
			report.diagnostics.push_back({ code, where });
	}
	void check_balance(economic_sql_native_holding &holding)
	{
		if (!holding.balance)
			return;
		if (std::any_of(holding.balance->begin(), holding.balance->end(),
				[](auto n) { return n < 0; }))
			observe(issue::negative_holding, holding.source);
		int64_t copper = 0;
		if (economic_coin_value(*holding.balance, &copper) != economic_accounting_error::ok)
			observe(issue::accounting_overflow, holding.source);
	}
	void balance(economic_sql_native_holding &holding, const std::vector<cell> &row,
		     size_t start, size_t count, bool is_unsigned)
	{
		economic_coin_vector values = {};
		bool unknown = false, overflow = false, negative = false;
		for (size_t i = 0; i < count; ++i)
		{
			if (!row[start + i])
			{
				unknown = true;
				continue;
			}
			if (is_unsigned)
			{
				const auto value = integer<uint64_t>(row[start + i]);
				if (value >
				    static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
					overflow = true;
				else
					values[i] = static_cast<int64_t>(value);
			}
			else
			{
				values[i] = integer<int64_t>(row[start + i]);
				negative |= values[i] < 0;
			}
		}
		if (unknown)
			observe(issue::unknown_money, holding.source);
		if (overflow)
			observe(issue::accounting_overflow, holding.source);
		if (!unknown && !overflow)
		{
			holding.balance = values;
			check_balance(holding);
		}
		else if (negative)
			observe(issue::negative_holding, holding.source);
	}
	void monetary(const char *name, kind type, size_t amount, size_t count, bool is_unsigned,
		      std::optional<size_t> revision)
	{
		const auto table = source(name);
		const auto &rows = input.tables[table].rows;
		std::optional<uint64_t> previous;
		for (size_t index = 0; index < rows.size(); ++index)
		{
			const auto &cells = rows[index].cells;
			economic_sql_native_holding holding;
			holding.source = reference(table, index);
			holding.kind = type;
			holding.disposition = disposition::current;
			holding.native_id = integer<uint64_t>(cells[0]);
			require(!previous || holding.native_id > *previous);
			previous = holding.native_id;
			if (!holding.native_id)
				observe(issue::invalid_identity, holding.source);
			if (type == kind::bank)
				holding.native_context = integer<int8_t>(cells[2]);
			if (revision)
				holding.native_revision = integer<uint64_t>(cells[*revision]);
			else
				observe(issue::unavailable_native_revision, holding.source);
			balance(holding, cells, amount, count, is_unsigned);
			report.holdings.push_back(std::move(holding));
		}
	}
	void auctions()
	{
		const auto table = source("auctions");
		const auto &rows = input.tables[table].rows;
		std::optional<uint64_t> previous;
		for (size_t index = 0; index < rows.size(); ++index)
		{
			const auto &cells = rows[index].cells;
			economic_sql_native_holding holding;
			holding.source = reference(table, index);
			holding.kind = kind::auction;
			holding.native_id = integer<uint64_t>(cells[0]);
			require(!previous || holding.native_id > *previous);
			previous = holding.native_id;
			if (!holding.native_id)
				observe(issue::invalid_identity, holding.source);
			holding.native_revision = integer<uint64_t>(cells[7]);
			const auto bidder = integer<int64_t>(cells[3]);
			require(cells[2].has_value());
			if (*cells[2] == "CLOSED")
				holding.disposition = disposition::history;
			else if (*cells[2] == "OPEN" && bidder > 0)
			{
				holding.disposition = disposition::current;
				balance(holding, cells, 4, 1, true);
			}
			else if (bidder == 0 && (*cells[2] == "OPEN" || *cells[2] == "REMOVED"))
				holding.disposition = disposition::not_holding;
			else
			{
				holding.disposition = disposition::unresolved;
				observe(issue::unresolved_auction, holding.source);
				balance(holding, cells, 4, 1, true);
			}
			report.holdings.push_back(std::move(holding));
		}
	}
	void custody()
	{
		using key = std::tuple<uint8_t, uint64_t, uint64_t>;
		std::map<key, uint64_t> owners;
		auto table = source("item_owner_revision");
		for (size_t index = 0; index < input.tables[table].rows.size(); ++index)
		{
			const auto &cells = input.tables[table].rows[index].cells;
			economic_sql_native_owner owner;
			owner.source = reference(table, index);
			owner.owner = { static_cast<item_owner_type>(integer<uint8_t>(cells[0])),
					integer<uint64_t>(cells[1]), integer<uint64_t>(cells[2]) };
			owner.revision = integer<uint64_t>(cells[3]);
			if (!item_owner_identity_valid(owner.owner))
				observe(issue::invalid_custody, owner.source);
			require(owners.emplace(key{ static_cast<uint8_t>(owner.owner.type),
						    owner.owner.id, owner.owner.context_id },
					       owner.revision)
					.second);
			report.owners.push_back(owner);
		}
		table = source("item_uid_allocator");
		if (input.tables[table].rows.size() == 1 &&
		    integer<uint64_t>(input.tables[table].rows[0].cells[0]) == 1 &&
		    integer<uint64_t>(input.tables[table].rows[0].cells[1]) > 0)
			report.next_uid = integer<uint64_t>(input.tables[table].rows[0].cells[1]);
		else
			observe(issue::allocator_missing_or_invalid, reference(table, SIZE_MAX));
		table = source("item_current_owner");
		std::optional<uint64_t> previous;
		for (size_t index = 0; index < input.tables[table].rows.size(); ++index)
		{
			const auto &cells = input.tables[table].rows[index].cells;
			economic_sql_native_item item;
			item.source = reference(table, index);
			item.item.uid = integer<uint64_t>(cells[0]);
			require(!previous || item.item.uid > *previous);
			previous = item.item.uid;
			auto &position = item.item.position;
			position.root_uid = integer<uint64_t>(cells[1]);
			position.parent_uid = cells[2] ? integer<uint64_t>(cells[2]) : 0;
			position.owner = { static_cast<item_owner_type>(integer<uint8_t>(cells[3])),
					   integer<uint64_t>(cells[4]),
					   integer<uint64_t>(cells[5]) };
			position.revision = integer<uint64_t>(cells[6]);
			item.vnum = integer<int32_t>(cells[7]);
			position.state =
				static_cast<item_custody_state>(integer<uint8_t>(cells[8]));
			if (!item.item.uid || !position.root_uid ||
			    (cells[2] && !position.parent_uid) || item.vnum <= 0 ||
			    !item_owner_identity_valid(position.owner) ||
			    position.state < item_custody_state::active ||
			    position.state > item_custody_state::quarantined)
				observe(issue::invalid_custody, item.source);
			if (position.state == item_custody_state::quarantined)
				observe(issue::quarantined_item, item.source);
			if (report.next_uid && (item.item.uid >= *report.next_uid ||
						position.root_uid >= *report.next_uid ||
						position.parent_uid >= *report.next_uid))
				observe(issue::uid_outside_allocator, item.source);
			auto owner =
				owners.find(key{ static_cast<uint8_t>(position.owner.type),
						 position.owner.id, position.owner.context_id });
			if (owner == owners.end())
				observe(issue::missing_owner_revision, item.source);
			else
				item.owner_revision = owner->second;
			if (!cells[9])
			{
				if (position.state == item_custody_state::active)
					observe(issue::unknown_coin_payload, item.source);
			}
			else
			{
				const auto &blob = *cells[9];
				std::vector<player_item_snapshot> decoded;
				auto result = player_snapshot_codec_result::invalid_value;
				if (!blob.empty() &&
				    blob.size() <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES)
					result = player_item_snapshot_list_decode(
						reinterpret_cast<const uint8_t *>(blob.data()),
						blob.size(), &decoded);
				if (result == player_snapshot_codec_result::allocation_failure)
					throw failure{ economic_accounting_error::capacity };
				if (result != player_snapshot_codec_result::ok ||
				    decoded.size() != 1 || decoded[0].object_uid != item.item.uid ||
				    decoded[0].vnum != item.vnum || decoded[0].type != ITEM_MONEY)
					observe(issue::invalid_coin_payload, item.source);
				else
				{
					economic_coin_vector coins = {};
					std::copy_n(decoded[0].values.begin(), 4, coins.begin());
					item.coin_values = coins;
					economic_sql_native_holding holding;
					holding.source = item.source;
					holding.kind = kind::pile;
					holding.native_id = item.item.uid;
					holding.native_revision = position.revision;
					holding.balance = coins;
					holding.disposition =
						position.state == item_custody_state::active ?
							disposition::current :
							(position.state == item_custody_state::
										   destroyed ?
								 disposition::history :
								 disposition::unresolved);
					check_balance(holding);
					report.holdings.push_back(std::move(holding));
				}
			}
			report.items.push_back(std::move(item));
		}
	}
	void pending()
	{
		for (const auto *name :
		     { "item_ownership_quarantine", "auction_reconciliation_quarantine",
		       "collector_reconciliation_quarantine" })
		{
			const auto table = source(name);
			for (size_t row = 0; row < input.tables[table].rows.size(); ++row)
			{
				const auto repaired = integer<uint8_t>(
					input.tables[table].rows[row].cells.back());
				require(repaired <= 1);
				if (!repaired)
					observe(issue::open_quarantine, reference(table, row));
			}
		}
		auto table = source("critical_operation_inbox");
		for (size_t row = 0; row < input.tables[table].rows.size(); ++row)
		{
			const auto &cells = input.tables[table].rows[row].cells;
			const auto complete = integer<uint8_t>(cells[11]);
			require(complete <= 1);
			if (integer<uint8_t>(cells[6]) != 1 || !complete)
				observe(issue::incomplete_receipt, reference(table, row));
		}
		table = source("critical_outbox");
		for (size_t row = 0; row < input.tables[table].rows.size(); ++row)
			if (integer<uint8_t>(input.tables[table].rows[row].cells[7]) != 1)
				observe(issue::pending_publication, reference(table, row));
		table = source("auction_item_pickups");
		for (size_t row = 0; row < input.tables[table].rows.size(); ++row)
			if (integer<uint8_t>(input.tables[table].rows[row].cells[3]) != 1)
				observe(issue::legacy_item_claim, reference(table, row));
	}
	void run()
	{
		report.source_digest = input.digest;
		report.diagnostics.reserve(limit);
		monetary("player_data", kind::wallet, 3, 4, false, 7);
		monetary("account_banks", kind::bank, 3, 4, true, 7);
		monetary("ships", kind::ship, 2, 1, false, std::nullopt);
		monetary("auction_money_pickups", kind::claim, 1, 1, true, 2);
		auctions();
		custody();
		pending();
		report.diagnostics_truncated = report.diagnostic_count > report.diagnostics.size();
	}
};
}
economic_accounting_error
economic_sql_normalize_sources(const economic_sql_source_snapshot &input, size_t limit,
			       economic_sql_normalized_sources *output) noexcept
{
	if (!output || !limit || limit > 512)
		return economic_accounting_error::corrupt_evidence;
	const auto valid = economic_sql_validate_sources(input);
	if (valid)
		return valid == E2BIG || valid == ENOMEM ?
			       economic_accounting_error::capacity :
			       economic_accounting_error::corrupt_evidence;
	try
	{
		consumer worker{ input, limit, {} };
		worker.run();
		*output = std::move(worker.report);
		return economic_accounting_error::ok;
	}
	catch (const failure &error)
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
