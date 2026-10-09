#include "economy/economic_sql_source_normalize.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <limits>
#include <map>
#include <set>
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
			if (input.version == 2)
			{
				const auto &equipment = input.item_equipment_sources[0].rows[index];
				item.observed_equipment_slot =
					integer<uint16_t>(equipment.cells[1]);
				position.equipment_slot = *item.observed_equipment_slot;
				item.equipment_source =
					economic_sql_equipment_source_reference{ index,
										 equipment.digest };
			}
			item.vnum = integer<int32_t>(cells[7]);
			position.state =
				static_cast<item_custody_state>(integer<uint8_t>(cells[8]));
			const bool native_mobile = position.owner.type ==
						   item_owner_type::native_mobile;
			const bool invalid_equipment =
				item.observed_equipment_slot &&
				((native_mobile &&
				  position.equipment_slot > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT) ||
				 (position.equipment_slot &&
				  ((position.owner.type != item_owner_type::player &&
				    !native_mobile) ||
				   position.parent_uid ||
				   position.state != item_custody_state::active ||
				   (native_mobile && position.root_uid != item.item.uid))));
			if (invalid_equipment || !item.item.uid || !position.root_uid ||
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
		report.custody_digest = input.custody_digest;
		report.diagnostics.reserve(limit);
		monetary("player_data", kind::wallet, 3, 4, false, 7);
		monetary("account_banks", kind::bank, 3, 4, true, 7);
		monetary("shopkeepers", kind::treasury, 4, 1, false, 5);
		const auto shops = source("shopkeepers");
		for (size_t row = 0; row < input.tables[shops].rows.size(); ++row)
		{
			const auto &roaming = input.tables[shops].rows[row].cells[6];
			if (!roaming)
				observe(issue::unknown_keeper_configuration, reference(shops, row));
			else
				require(integer<uint8_t>(roaming) <= 1);
		}
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

namespace
{
using physical_issue = economic_sql_physical_issue;
using registry = economic_sql_physical_registry;
using owner_key = std::tuple<uint8_t, uint64_t, uint64_t>;
owner_key owner_identity(const item_owner_identity &owner)
{
	return { static_cast<uint8_t>(owner.type), owner.id, owner.context_id };
}
template <class T> std::optional<T> observed(const cell &value)
{
	return value ? std::optional<T>{ integer<T>(value) } : std::nullopt;
}
struct physical_consumer
{
	const economic_sql_physical_source_snapshot &input;
	size_t limit;
	economic_sql_normalized_physical_sources report;
	struct location
	{
		registry kind;
		size_t table, parent, uid, vnum;
	};
	static constexpr location locations[] = {
		{ registry::physical_sources, 0, 2, 3, 4 }, // player
		{ registry::physical_sources, 1, 2, 3, 4 }, // corpse
		{ registry::physical_sources, 2, 3, 4, 5 }, // saved room
		{ registry::physical_sources, 3, 3, 4, 5 }, // private locker
		{ registry::physical_sources, 4, 2, 3, 4 }, // account locker
		{ registry::source2_item_sources, 0, 2, 3, 4 }, // pet
		{ registry::source2_item_sources, 1, 2, 3, 4 }, // shopkeeper
		{ registry::source2_item_sources, 2, 2, 3, 4 }, // siege room
	};
	struct lookup
	{
		size_t index;
		bool ambiguous = false;
	};
	using index_map = std::map<uint64_t, lookup>;
	std::array<index_map, 6> mappings;
	index_map players, shops;
	std::array<index_map, 8> physical_ids;
	std::array<size_t, 8> offsets = {};
	std::array<std::vector<std::optional<item_owner_identity>>, 3> lifetimes;
	std::array<std::vector<bool>, 3> ambiguous_lifetimes;
	struct public_chest
	{
		size_t row = SIZE_MAX, count = 0;
		bool uncertain = false;
	};
	std::map<uint64_t, public_chest> public_chests;
	std::vector<size_t> parents;
	std::vector<uint8_t> graph_valid;

	const std::vector<economic_sql_source_table> &tables(registry kind) const
	{
		switch (kind)
		{
		case registry::source2_tables:
			return input.source2.tables;
		case registry::source2_item_sources:
			return input.source2.item_sources;
		case registry::source2_item_equipment_sources:
			return input.source2.item_equipment_sources;
		case registry::physical_sources:
			return input.physical_sources;
		}
		throw failure{ economic_accounting_error::corrupt_evidence };
	}
	economic_sql_physical_reference reference(registry kind, size_t table, size_t row) const
	{
		const auto &selected = tables(kind)[table];
		return { kind, table, row,
			 row == SIZE_MAX ? selected.content_digest : selected.rows[row].digest };
	}
	const std::vector<cell> &cells(const economic_sql_physical_reference &ref) const
	{
		return tables(ref.registry)[ref.table].rows[ref.row].cells;
	}
	void observe(physical_issue code, const economic_sql_physical_reference &where)
	{
		++report.issue_counts[static_cast<size_t>(code)];
		++report.diagnostic_count;
		if (report.diagnostics.size() < limit)
			report.diagnostics.push_back({ code, where });
	}
	void mark(size_t index, physical_issue code)
	{
		auto &item = report.items[index];
		const auto mask = uint32_t{ 1 } << static_cast<uint8_t>(code);
		if (!(item.issue_mask & mask))
		{
			item.issue_mask |= mask;
			observe(code, item.source);
		}
	}
	index_map index(registry kind, size_t table, bool diagnose)
	{
		index_map result;
		const auto &rows = tables(kind)[table].rows;
		std::optional<uint64_t> previous;
		for (size_t row = 0; row < rows.size(); ++row)
		{
			const auto id = observed<uint64_t>(rows[row].cells[0]);
			if (!id || !*id || (previous && *id <= *previous))
				if (diagnose)
					observe(physical_issue::invalid_identity,
						reference(kind, table, row));
			if (id)
				previous = id;
			if (!id || !*id)
				continue;
			auto [found, inserted] = result.emplace(*id, lookup{ row });
			if (!inserted)
			{
				if (diagnose && !found->second.ambiguous)
					observe(physical_issue::invalid_identity,
						reference(kind, table, found->second.index));
				found->second.ambiguous = true;
			}
		}
		return result;
	}
	std::optional<size_t> mapped(const index_map &map, const std::optional<uint64_t> &id,
				     size_t item)
	{
		if (id && *id)
		{
			auto found = map.find(*id);
			if (found != map.end())
			{
				if (!found->second.ambiguous)
					return found->second.index;
				mark(item, physical_issue::ambiguous_owner);
				return std::nullopt;
			}
		}
		mark(item, physical_issue::unresolved_owner);
		return std::nullopt;
	}
	void resolve_lifetimes(size_t group, registry kind, size_t table)
	{
		const auto &rows = tables(kind)[table].rows;
		auto &resolved = lifetimes[group];
		resolved.resize(rows.size());
		ambiguous_lifetimes[group].resize(rows.size());
		std::map<uint64_t, lookup> identities;
		for (size_t row = 0; row < rows.size(); ++row)
		{
			const auto &value = rows[row].cells;
			item_owner_identity owner = {};
			bool valid = false;
			std::optional<uint64_t> observed_identity;
			if (group == 0)
			{
				const auto uid = observed<uint64_t>(value[1]);
				const auto pid = observed<uint32_t>(value[2]);
				if (uid && *uid)
					observed_identity = uid;
				if (uid && pid)
				{
					owner = { item_owner_type::pet, *uid, *pid };
					const auto player = players.find(*pid);
					valid = player != players.end() &&
						!player->second.ambiguous;
				}
			}
			else if (group == 1)
			{
				const auto save = observed<int64_t>(value[1]);
				const auto pid = observed<int32_t>(value[2]);
				const auto revision = observed<uint64_t>(value[3]);
				(void)observed<int32_t>(
					value[4]); // Raw room retained, not the owner key.
				valid = save && *save > 0 && *save <= INT32_MAX && pid &&
					*pid > 0 && revision && *revision;
				if (save && *save > 0 && *save <= INT32_MAX && pid && *pid > 0)
				{
					owner = { item_owner_type::corpse,
						  item_corpse_owner_id(static_cast<uint32_t>(*pid),
								       static_cast<uint32_t>(*save)),
						  0 };
					observed_identity = owner.id;
				}
			}
			else
			{
				const auto shop = observed<int32_t>(value[1]);
				valid = shop && *shop >= 0;
				if (valid)
				{
					owner = { item_owner_type::shopkeeper,
						  item_shopkeeper_owner_id(
							  static_cast<uint32_t>(*shop)),
						  0 };
					observed_identity = owner.id;
				}
			}
			valid = valid && item_owner_identity_valid(owner);
			if (!valid)
				observe(physical_issue::unresolved_owner,
					reference(kind, table, row));
			else
				resolved[row] = owner;
			if (!observed_identity)
				continue;
			// Pet UID is a lifetime regardless of owner PID. Corpse/shop IDs
			// already encode their complete resolved lifetime identity.
			auto [found, inserted] =
				identities.emplace(*observed_identity, lookup{ row });
			if (!inserted)
			{
				ambiguous_lifetimes[group][row] = true;
				ambiguous_lifetimes[group][found->second.index] = true;
				observe(physical_issue::ambiguous_owner,
					reference(kind, table, row));
				if (!found->second.ambiguous)
					observe(physical_issue::ambiguous_owner,
						reference(kind, table, found->second.index));
				found->second.ambiguous = true;
			}
		}
	}
	void prepare_mappings()
	{
		players = index(registry::source2_tables, 0, false);
		shops = index(registry::source2_tables, 2, false);
		for (size_t table = 5; table <= 10; ++table)
			mappings[table - 5] = index(registry::physical_sources, table, true);
		resolve_lifetimes(0, registry::physical_sources, 5);
		resolve_lifetimes(1, registry::physical_sources, 6);
		resolve_lifetimes(2, registry::source2_tables, 2);
		const auto &chests = input.physical_sources[7].rows;
		for (size_t row = 0; row < chests.size(); ++row)
		{
			const auto locker = observed<uint32_t>(chests[row].cells[1]);
			const auto is_public = observed<int32_t>(chests[row].cells[2]);
			if (!locker || !*locker)
			{
				observe(physical_issue::invalid_identity,
					reference(registry::physical_sources, 7, row));
				continue;
			}
			const auto native_locker = mappings[3].find(*locker);
			if (native_locker == mappings[3].end() || native_locker->second.ambiguous)
				observe(physical_issue::unresolved_owner,
					reference(registry::physical_sources, 7, row));
			auto &entry = public_chests[*locker];
			if (!is_public || *is_public < 0 || *is_public > 1)
			{
				entry.uncertain = true;
				observe(physical_issue::unresolved_owner,
					reference(registry::physical_sources, 7, row));
			}
			else if (*is_public == 1)
			{
				++entry.count;
				entry.row = row;
			}
		}
		for (size_t row = 0; row < input.physical_sources[9].rows.size(); ++row)
		{
			const auto locker =
				observed<uint32_t>(input.physical_sources[9].rows[row].cells[1]);
			const auto native_locker = locker ? mappings[5].find(*locker) :
							    mappings[5].end();
			if (!locker || !*locker || native_locker == mappings[5].end() ||
			    native_locker->second.ambiguous)
				observe(physical_issue::unresolved_owner,
					reference(registry::physical_sources, 9, row));
		}
	}
	void lifetime_owner(size_t item, size_t group, registry kind, size_t table,
			    const index_map &map, const std::optional<uint64_t> &id)
	{
		const auto row = mapped(map, id, item);
		if (!row)
			return;
		auto &record = report.items[item];
		record.mapping_sources[0] = reference(kind, table, *row);
		if (ambiguous_lifetimes[group][*row])
			mark(item, physical_issue::ambiguous_owner);
		if (!lifetimes[group][*row])
			mark(item, physical_issue::unresolved_owner);
		else if (!ambiguous_lifetimes[group][*row])
		{
			record.owner = lifetimes[group][*row];
			if (group == 0)
			{
				const auto player = players.find(record.owner->context_id);
				record.mapping_sources[1] = reference(registry::source2_tables, 0,
								      player->second.index);
			}
		}
	}
	void owner(size_t source, size_t item)
	{
		auto &record = report.items[item];
		const auto &value = cells(record.source);
		if (source == 0)
		{
			const auto pid = observed<uint32_t>(value[1]);
			const auto row =
				mapped(players,
				       pid ? std::optional<uint64_t>{ *pid } : std::nullopt, item);
			if (row)
			{
				record.owner =
					item_owner_identity{ item_owner_type::player, *pid, 0 };
				record.mapping_sources[0] =
					reference(registry::source2_tables, 0, *row);
			}
		}
		else if (source == 1)
			lifetime_owner(item, 1, registry::physical_sources, 6, mappings[1],
				       observed<uint64_t>(value[1]));
		else if (source == 5)
			lifetime_owner(item, 0, registry::physical_sources, 5, mappings[0],
				       observed<uint64_t>(value[1]));
		else if (source == 6)
			lifetime_owner(item, 2, registry::source2_tables, 2, shops,
				       observed<uint64_t>(value[1]));
		else if (source == 2 || source == 7)
		{
			const auto room = observed<int32_t>(value[source == 2 ? 2 : 1]);
			if (room && *room > 0)
				record.owner = item_owner_identity{ item_owner_type::room,
								    static_cast<uint64_t>(*room),
								    0 };
			else
				mark(item, physical_issue::unresolved_owner);
		}
		else if (source == 3)
		{
			const auto locker = observed<uint32_t>(value[1]);
			const auto locker_row = mapped(
				mappings[3],
				locker ? std::optional<uint64_t>{ *locker } : std::nullopt, item);
			const auto chest_id = observed<uint32_t>(value[2]);
			std::optional<size_t> chest;
			if (chest_id)
				chest = mapped(mappings[2], std::optional<uint64_t>{ *chest_id },
					       item);
			else if (locker)
			{
				const auto found = public_chests.find(*locker);
				if (found != public_chests.end() && found->second.count == 1 &&
				    !found->second.uncertain)
					chest = found->second.row;
				else
					mark(item, found != public_chests.end() &&
								   (found->second.count > 1 ||
								    found->second.uncertain) ?
							   physical_issue::ambiguous_owner :
							   physical_issue::unresolved_owner);
			}
			if (locker_row)
				record.mapping_sources[0] =
					reference(registry::physical_sources, 8, *locker_row);
			if (chest)
			{
				record.mapping_sources[1] =
					reference(registry::physical_sources, 7, *chest);
				const auto &mapping = input.physical_sources[7].rows[*chest].cells;
				const auto mapped_locker = observed<uint32_t>(mapping[1]);
				const auto identity = observed<uint32_t>(mapping[0]);
				if (locker_row && locker && mapped_locker == locker && identity &&
				    *identity && !mappings[2].at(*identity).ambiguous)
					record.owner = item_owner_identity{ item_owner_type::locker,
									    *locker, *identity };
				else
					mark(item, physical_issue::unresolved_owner);
			}
		}
		else
		{
			const auto chest_id = observed<uint32_t>(value[1]);
			const auto chest = mapped(mappings[4],
						  chest_id ? std::optional<uint64_t>{ *chest_id } :
							     std::nullopt,
						  item);
			if (chest)
			{
				record.mapping_sources[0] =
					reference(registry::physical_sources, 9, *chest);
				const auto locker = observed<uint32_t>(
					input.physical_sources[9].rows[*chest].cells[1]);
				const auto locker_row = mapped(
					mappings[5],
					locker ? std::optional<uint64_t>{ *locker } : std::nullopt,
					item);
				if (locker_row)
				{
					record.mapping_sources[1] = reference(
						registry::physical_sources, 10, *locker_row);
					record.owner = item_owner_identity{ item_owner_type::locker,
									    *chest_id, 0 };
				}
			}
		}
		if (record.owner && !item_owner_identity_valid(*record.owner))
		{
			record.owner.reset();
			mark(item, physical_issue::unresolved_owner);
		}
	}
	void read_items()
	{
		uint64_t count = 0;
		for (const auto &where : locations)
			count += tables(where.kind)[where.table].rows.size();
		require(count <= input.rows);
		report.items.reserve(count);
		parents.reserve(count);
		graph_valid.reserve(count);
		for (size_t source = 0; source < std::size(locations); ++source)
		{
			const auto &where = locations[source];
			const auto &rows = tables(where.kind)[where.table].rows;
			offsets[source] = report.items.size();
			physical_ids[source] = index(where.kind, where.table, false);
			std::optional<uint64_t> previous;
			for (size_t row = 0; row < rows.size(); ++row)
			{
				const auto item = report.items.size();
				economic_sql_physical_item record;
				record.source = reference(where.kind, where.table, row);
				record.row_id = observed<uint64_t>(rows[row].cells[0]);
				record.uid = observed<uint64_t>(rows[row].cells[where.uid]);
				record.vnum = observed<int32_t>(rows[row].cells[where.vnum]);
				report.items.push_back(std::move(record));
				parents.push_back(SIZE_MAX);
				graph_valid.push_back(1);
				auto &entry = report.items.back();
				if (!entry.row_id || !*entry.row_id ||
				    (previous && *entry.row_id <= *previous) ||
				    (entry.row_id && *entry.row_id &&
				     physical_ids[source].at(*entry.row_id).ambiguous) ||
				    !entry.vnum || *entry.vnum <= 0)
				{
					mark(item, physical_issue::invalid_identity);
					graph_valid[item] = 0;
				}
				if (entry.row_id)
					previous = entry.row_id;
				if (!entry.uid || !*entry.uid)
				{
					mark(item, physical_issue::missing_uid);
					graph_valid[item] = 0;
				}
				if (source == 2 && !rows[row].cells[1])
				{
					mark(item, physical_issue::invalid_identity);
					graph_valid[item] = 0;
				}
				owner(source, item);
				if (!entry.owner)
					graph_valid[item] = 0;
				if (source == 0 || source == 5 || source == 6)
				{
					const auto equipment =
						source == 0 ? entry.source :
							      reference(registry::physical_sources,
									source == 5 ? 11 : 12, row);
					entry.equipment_source = equipment;
					const auto slot = observed<int32_t>(
						cells(equipment)[source == 0 ? 5 : 1]);
					if (!slot)
						mark(item, physical_issue::unknown_equipment);
					else
					{
						if (*slot < 0 ||
						    *slot > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT)
							mark(item,
							     physical_issue::invalid_equipment);
						else
							entry.observed_equipment_slot =
								static_cast<uint16_t>(*slot);
						if (*slot && rows[row].cells[where.parent])
							mark(item,
							     physical_issue::invalid_equipment);
					}
					if (source != 0 && slot && *slot > 0)
						mark(item, physical_issue::unsupported_equipment);
				}
				(void)observed<uint64_t>(rows[row].cells[where.parent]);
			}
		}
	}
	void duplicates()
	{
		std::map<uint64_t, lookup> uids;
		std::map<std::pair<owner_key, uint16_t>, lookup> slots;
		std::map<owner_key, std::pair<size_t, size_t>> locker_namespaces;
		for (size_t source = 0; source < std::size(locations); ++source)
		{
			const auto end = source + 1 < std::size(locations) ? offsets[source + 1] :
									     report.items.size();
			for (size_t item = offsets[source]; item < end; ++item)
			{
				const auto &entry = report.items[item];
				if (entry.uid && *entry.uid)
				{
					auto [found, inserted] =
						uids.emplace(*entry.uid, lookup{ item });
					if (!inserted)
					{
						mark(found->second.index,
						     physical_issue::duplicate_uid);
						mark(item, physical_issue::duplicate_uid);
						graph_valid[found->second.index] = 0;
						graph_valid[item] = 0;
					}
				}
				if (entry.owner && entry.observed_equipment_slot &&
				    *entry.observed_equipment_slot)
				{
					auto [found, inserted] = slots.emplace(
						std::pair{ owner_identity(*entry.owner),
							   *entry.observed_equipment_slot },
						lookup{ item });
					if (!inserted)
					{
						mark(found->second.index,
						     physical_issue::duplicate_equipment);
						mark(item, physical_issue::duplicate_equipment);
					}
				}
				if ((source == 3 || source == 4) && entry.owner)
				{
					auto [found, inserted] = locker_namespaces.emplace(
						owner_identity(*entry.owner),
						std::pair{ source, item });
					if (!inserted && found->second.first != source)
					{
						mark(found->second.second,
						     physical_issue::ambiguous_owner);
						mark(item, physical_issue::ambiguous_owner);
					}
				}
			}
		}
	}
	bool same_scope(size_t source, const economic_sql_physical_item &child,
			const economic_sql_physical_item &parent) const
	{
		if (!child.owner || !parent.owner ||
		    !item_owner_identity_equal(*child.owner, *parent.owner))
			return false;
		const auto &left = cells(child.source), &right = cells(parent.source);
		return left[1] == right[1] && (source != 2 || left[2] == right[2]);
	}
	void topology()
	{
		for (size_t source = 0; source < std::size(locations); ++source)
		{
			const auto &where = locations[source];
			const auto end = source + 1 < std::size(locations) ? offsets[source + 1] :
									     report.items.size();
			for (size_t item = offsets[source]; item < end; ++item)
			{
				auto &entry = report.items[item];
				const auto parent_id =
					observed<uint64_t>(cells(entry.source)[where.parent]);
				if (!parent_id)
				{
					entry.parent_uid = 0; // Observed SQL NULL denotes a root.
					continue;
				}
				const auto found = physical_ids[source].find(*parent_id);
				if (!*parent_id || found == physical_ids[source].end() ||
				    found->second.ambiguous)
				{
					mark(item, physical_issue::unresolved_parent);
					graph_valid[item] = 0;
					continue;
				}
				const auto parent = offsets[source] + found->second.index;
				entry.parent_source = report.items[parent].source;
				if (!same_scope(source, entry, report.items[parent]))
				{
					mark(item, physical_issue::cross_scope_parent);
					graph_valid[item] = 0;
				}
				if (!report.items[parent].uid || !*report.items[parent].uid)
				{
					mark(item, physical_issue::unresolved_parent);
					graph_valid[item] = 0;
				}
				else
					entry.parent_uid = report.items[parent].uid;
				parents[item] = parent;
			}
		}
		// One iterative color walk per record. The reusable path holds row
		// indices only; no recursion or copied ancestor chains/SQL cells.
		std::vector<uint8_t> color(report.items.size());
		std::vector<size_t> path;
		path.reserve(report.items.size());
		for (size_t start = 0; start < report.items.size(); ++start)
		{
			if (color[start])
				continue;
			path.clear();
			size_t cursor = start;
			while (cursor != SIZE_MAX && !color[cursor])
			{
				color[cursor] = 1;
				path.push_back(cursor);
				cursor = graph_valid[cursor] ? parents[cursor] : SIZE_MAX;
			}
			const bool cycle = cursor != SIZE_MAX && color[cursor] == 1;
			for (auto item = path.rbegin(); item != path.rend(); ++item)
			{
				auto &entry = report.items[*item];
				if (!cycle && graph_valid[*item])
					entry.root_uid =
						parents[*item] == SIZE_MAX ?
							entry.uid :
							report.items[parents[*item]].root_uid;
				if (!entry.root_uid)
					mark(*item, physical_issue::invalid_topology);
				color[*item] = 2;
			}
		}
	}
	void correspond()
	{
		std::map<uint64_t, size_t> native;
		for (size_t index = 0; index < report.source2.items.size(); ++index)
			native.emplace(report.source2.items[index].item.uid, index);
		std::vector<uint8_t> matched(report.source2.items.size());
		for (size_t item = 0; item < report.items.size(); ++item)
		{
			auto &entry = report.items[item];
			const auto found = entry.uid ? native.find(*entry.uid) : native.end();
			if (found == native.end())
			{
				mark(item, physical_issue::unmatched_physical);
				continue;
			}
			entry.custody_index = found->second;
			const auto &custody = report.source2.items[found->second];
			const auto &position = custody.item.position;
			const bool equipment_equal =
				entry.equipment_source ? entry.observed_equipment_slot &&
								 *entry.observed_equipment_slot ==
									 position.equipment_slot :
							 position.equipment_slot == 0;
			const bool equal =
				entry.uid && *entry.uid && entry.owner && entry.root_uid &&
				entry.parent_uid && entry.vnum &&
				position.state == item_custody_state::active &&
				item_owner_identity_equal(*entry.owner, position.owner) &&
				*entry.root_uid == position.root_uid &&
				*entry.parent_uid == position.parent_uid &&
				*entry.vnum == custody.vnum && equipment_equal;
			if (!equal)
				mark(item, physical_issue::conflicting_custody);
			if (equal && !entry.issue_mask)
			{
				entry.exact_custody_match = true;
				matched[found->second] = 1;
			}
		}
		for (size_t index = 0; index < report.source2.items.size(); ++index)
		{
			const auto &item = report.source2.items[index];
			if (item.item.position.state == item_custody_state::active &&
			    !matched[index])
			{
				report.unmatched_active_custody_indices.push_back(index);
				observe(physical_issue::unmatched_active_custody,
					reference(registry::source2_tables, item.source.table,
						  item.source.row));
			}
		}
	}
	void run()
	{
		report.physical_digest = input.digest;
		report.diagnostics.reserve(limit);
		prepare_mappings();
		read_items();
		duplicates();
		topology();
		correspond();
		report.diagnostics_truncated = report.diagnostic_count > report.diagnostics.size();
	}
};
static_assert(static_cast<size_t>(physical_issue::count) <= 32);
}

economic_accounting_error
economic_sql_normalize_physical_sources(const economic_sql_physical_source_snapshot &input,
					size_t limit,
					economic_sql_normalized_physical_sources *output) noexcept
{
	if (!output || !limit || limit > 512)
		return economic_accounting_error::corrupt_evidence;
	const auto valid = economic_sql_validate_physical_sources(input);
	if (valid)
		return valid == E2BIG || valid == ENOMEM ?
			       economic_accounting_error::capacity :
			       economic_accounting_error::corrupt_evidence;
	try
	{
		physical_consumer worker{ input, limit, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {} };
		const auto native = economic_sql_normalize_sources(input.source2, limit,
								   &worker.report.source2);
		if (native != economic_accounting_error::ok)
			return native;
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

namespace
{
economic_accounting_error persisted_provider_failure(unsigned int code)
{
	return code == E2BIG || code == ENOMEM ? economic_accounting_error::capacity :
	       code == ENOTSUP		       ? economic_accounting_error::unresolved :
						 economic_accounting_error::corrupt_evidence;
}
}

economic_accounting_error economic_sql_normalize_persisted_correspondence(
	const economic_sql_physical_source_snapshot &base,
	const sql_room_item_source_snapshot &supplement, const economic_sql_source_limits &limits,
	size_t diagnostic_limit, economic_sql_persisted_correspondence *output) noexcept
{
	if (!output || !diagnostic_limit || diagnostic_limit > 512)
		return economic_accounting_error::corrupt_evidence;
	try
	{
		const auto framing = economic_sql_validate_physical_sources(base, limits);
		if (framing)
			return persisted_provider_failure(framing);
		economic_sql_persisted_correspondence result;
		const auto physical = economic_sql_normalize_physical_sources(
			base, diagnostic_limit, &result.physical);
		if (physical != economic_accounting_error::ok)
			return physical;
		auto code = sql_room_item_payload_inspect_sources(base, supplement.tables, limits,
								  diagnostic_limit, &result.room);
		if (code)
			return persisted_provider_failure(code);
		code = auction_repository_inspect_physical_sources(
			base, supplement, limits, diagnostic_limit, &result.auction);
		if (code)
			return persisted_provider_failure(code);
		code = collector_repository_inspect_physical_sources(base, &result.collector,
								     limits, diagnostic_limit);
		if (code)
			return persisted_provider_failure(code);

		const auto &items = result.physical.source2.items;
		result.custody.resize(items.size());
		std::map<size_t, size_t> by_raw_row;
		for (size_t index = 0; index < items.size(); ++index)
		{
			result.custody[index].custody_index = index;
			if (items[index].source.table != 11 ||
			    !by_raw_row.emplace(items[index].source.row, index).second)
				return economic_accounting_error::corrupt_evidence;
		}
		auto add =
			[&](size_t index, economic_sql_persisted_provider provider, size_t witness)
		{
			if (index >= items.size() ||
			    items[index].item.position.state != item_custody_state::active)
				return false;
			result.custody[index].matches.push_back({ provider, witness });
			return true;
		};
		auto add_raw =
			[&](size_t row, economic_sql_persisted_provider provider, size_t witness)
		{
			const auto found = by_raw_row.find(row);
			return found != by_raw_row.end() && add(found->second, provider, witness);
		};
		for (size_t index = 0; index < result.physical.items.size(); ++index)
		{
			const auto &witness = result.physical.items[index];
			if (witness.exact_custody_match &&
			    (!witness.custody_index ||
			     !add(*witness.custody_index, economic_sql_persisted_provider::physical,
				  index)))
				return economic_accounting_error::corrupt_evidence;
		}
		// A malformed graph never supplies a current counterpart. The original
		// whole-root-count refusal remains visible separately in result.room.
		for (const auto &graph : result.room.graphs)
			if (graph.valid)
				for (size_t index : graph.witness_indices)
				{
					if (index >= result.room.witnesses.size() ||
					    !add_raw(result.room.witnesses[index].custody_row,
						     economic_sql_persisted_provider::room, index))
						return economic_accounting_error::corrupt_evidence;
				}
		for (size_t index = 0; index < result.auction.nodes.size(); ++index)
		{
			const auto &witness = result.auction.nodes[index];
			if (!witness.current_field_correspondence)
				continue;
			if (witness.slot_index >= result.auction.slots.size())
				return economic_accounting_error::corrupt_evidence;
			const auto &slot = result.auction.slots[witness.slot_index];
			if (!slot.claimed || *slot.claimed || !witness.custody ||
			    witness.custody->supplemental || witness.custody->equipment ||
			    witness.custody->table != 11 ||
			    !add_raw(witness.custody->row, economic_sql_persisted_provider::auction,
				     index))
				return economic_accounting_error::corrupt_evidence;
		}
		for (size_t index = 0; index < result.auction.legacy_identities.size(); ++index)
		{
			const auto &witness = result.auction.legacy_identities[index];
			if (!witness.current_field_correspondence)
				continue;
			if (witness.slot_index >= result.auction.slots.size())
				return economic_accounting_error::corrupt_evidence;
			const auto &slot = result.auction.slots[witness.slot_index];
			if (!slot.claimed || *slot.claimed || !witness.custody ||
			    witness.custody->supplemental || witness.custody->equipment ||
			    witness.custody->table != 11 ||
			    !add_raw(witness.custody->row,
				     economic_sql_persisted_provider::auction_legacy_identity,
				     index))
				return economic_accounting_error::corrupt_evidence;
		}
		for (size_t index = 0; index < result.collector.listings.size(); ++index)
		{
			const auto &witness = result.collector.listings[index];
			if (witness.held && witness.correspondence_valid &&
			    !add_raw(witness.custody_row,
				     economic_sql_persisted_provider::collector, index))
				return economic_accounting_error::corrupt_evidence;
		}
		for (size_t index = 0; index < items.size(); ++index)
			if (items[index].item.position.state == item_custody_state::active)
			{
				if (result.custody[index].matches.empty())
					result.unmatched_active_custody_indices.push_back(index);
				else if (result.custody[index].matches.size() > 1)
					result.multiply_matched_active_custody_indices.push_back(
						index);
			}
		*output = std::move(result);
		return economic_accounting_error::ok;
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

economic_accounting_error economic_sql_normalize_persisted_correspondence(
	const economic_sql_physical_source_snapshot &base,
	const sql_room_item_source_snapshot &supplement,
	const sql_room_creation_source_snapshot &creation, const economic_sql_source_limits &limits,
	size_t diagnostic_limit, economic_sql_persisted_correspondence *output) noexcept
{
	if (!output || !diagnostic_limit || diagnostic_limit > 512)
		return economic_accounting_error::corrupt_evidence;
	try
	{
		economic_sql_persisted_correspondence result;
		const auto original = economic_sql_normalize_persisted_correspondence(
			base, supplement, limits, diagnostic_limit, &result);
		if (original != economic_accounting_error::ok)
			return original;
		const auto inspected = sql_room_creation_correspondence_inspect(
			base, supplement, creation, limits, diagnostic_limit, &result.creation);
		if (inspected)
			return persisted_provider_failure(inspected);
		result.creation_observed = true;
		std::set<size_t> superseded;
		for (const auto index : result.creation.superseded_room_witness_indices)
			if (index >= result.room.witnesses.size() ||
			    !superseded.insert(index).second)
				return economic_accounting_error::corrupt_evidence;
		const auto &items = result.physical.source2.items;
		std::map<size_t, size_t> by_raw_row;
		for (size_t index = 0; index < items.size(); ++index)
		{
			if (items[index].source.table != 11 ||
			    !by_raw_row.emplace(items[index].source.row, index).second)
				return economic_accounting_error::corrupt_evidence;
			auto &matches = result.custody[index].matches;
			matches.erase(
				std::remove_if(
					matches.begin(), matches.end(),
					[&](const auto &match)
					{
						return match.provider ==
							       economic_sql_persisted_provider::room &&
						       superseded.count(match.witness_index);
					}),
				matches.end());
		}
		for (const auto &graph : result.creation.graphs)
			if (graph.valid)
				for (const auto index : graph.witness_indices)
				{
					if (index >= result.creation.witnesses.size())
						return economic_accounting_error::corrupt_evidence;
					const auto &witness = result.creation.witnesses[index];
					const auto found =
						by_raw_row.find(witness.room.custody_row);
					if (!witness.exact_current || found == by_raw_row.end() ||
					    items[found->second].item.position.state !=
						    item_custody_state::active)
						return economic_accounting_error::corrupt_evidence;
					result.custody[found->second].matches.push_back(
						{ economic_sql_persisted_provider::room_creation,
						  index });
				}
		result.unmatched_active_custody_indices.clear();
		result.multiply_matched_active_custody_indices.clear();
		for (size_t index = 0; index < items.size(); ++index)
			if (items[index].item.position.state == item_custody_state::active)
			{
				if (result.custody[index].matches.empty())
					result.unmatched_active_custody_indices.push_back(index);
				else if (result.custody[index].matches.size() > 1)
					result.multiply_matched_active_custody_indices.push_back(
						index);
			}
		*output = std::move(result);
		return economic_accounting_error::ok;
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
