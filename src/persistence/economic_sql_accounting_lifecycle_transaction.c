#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include "persistence/critical_command_coordinator.h"
#include "world/economic_initialized_world_owner.h"
#include "economy/economic_initialized_world_money_correspondence.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
#include "economy/native_mobile_birth_accounting.h"
#include "economy/economic_baseline_command.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_command.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/economic_sql_source_normalize.h"
#include "economy/economic_sql_runtime_cache_correspondence.h"
#include "persistence/economic_sql_baseline_transaction.h"
#include "persistence/economic_sql_auction_source_claim.h"
#include "persistence/economic_accounting_repository.h"
#include "persistence/economic_sql_pending_claim_source.h"
#include "persistence/sql_room_creation_source.h"
#include "persistence/zone_reset_creation_budget.h"
#include "world/vnum.obj.h"
#include "player/player_sql_transaction_cleanup.h"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <openssl/sha.h>
#include <set>
#include <string_view>
#include <type_traits>
#include <tuple>

#ifdef __NO_MYSQL__
unsigned int economic_sql_accounting_lifecycle_transaction::install(
	MYSQL *, const economic_sql_lifecycle_guard &, const economic_sql_lifecycle_request &,
	economic_sql_lifecycle_receipt *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::activate(
	MYSQL *, economic_sql_cutover_transaction_owner &, const critical_operation_id &,
	uint64_t *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::activate_verified(
	MYSQL *, economic_sql_cutover_transaction_owner &, const economic_sql_lifecycle_request &,
	const economic_sql_activation_evidence &, economic_sql_activation_verifier) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::pause(
	MYSQL *, economic_sql_cutover_transaction_owner &, const critical_operation_id &) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::recover_runtime(
	MYSQL *, const economic_sql_lifecycle_guard &, bool *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::prepare_runtime_boot(
	MYSQL *, const economic_sql_lifecycle_guard &,
	economic_sql_runtime_boot_selection *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::finish_runtime_boot(
	MYSQL *, const economic_sql_lifecycle_guard &, economic_sql_runtime_boot_selection &,
	bool *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::verify_aborted_runtime_projection(
	MYSQL *, economic_sql_cutover_transaction_owner &,
	const economic_sql_runtime_boot_selection &) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::verify_returned_runtime_projection(
	MYSQL *, const economic_sql_lifecycle_guard &,
	const economic_sql_runtime_boot_selection &) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::verify_runtime_return_projection(
	MYSQL *, economic_sql_cutover_transaction_owner *, const economic_sql_lifecycle_guard *,
	const economic_sql_runtime_boot_selection &) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_accounting_lifecycle_transaction::verify_initialized_runtime_sources(
	MYSQL *, economic_sql_cutover_transaction_owner &,
	const economic_sql_runtime_boot_selection &, const economic_sql_lifecycle_request &,
	const economic_sql_activation_evidence &,
	economic_sql_initialized_activation_verifier) noexcept
{
	return ENOTSUP;
}
#else
namespace
{
struct failure
{
	unsigned int code;
};
void require(bool valid, unsigned int code = EILSEQ)
{
	if (!valid)
		throw failure{ code };
}
using result_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
using row = std::vector<std::optional<std::string>>;
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
}
std::string binary(std::span<const uint8_t> value)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string text = "X'";
	text.reserve(value.size() * 2 + 3);
	for (auto byte : value)
	{
		text += digits[byte >> 4];
		text += digits[byte & 15];
	}
	text += '\'';
	return text;
}
std::string id(const critical_operation_id &value)
{
	return binary(value.bytes);
}
std::string digest_sql(const economic_sql_source_digest &value)
{
	return binary(value);
}
economic_sql_source_digest sha(std::span<const uint8_t> bytes)
{
	economic_sql_source_digest value = {};
	SHA256(bytes.data(), bytes.size(), value.data());
	return value;
}
struct holding_source
{
	economic_account_kind account_kind = economic_account_kind::wallet;
	uint64_t native_id = 0;
	uint8_t racewar = 0;
	uint32_t shop_id = 0;
	std::string name;
	economic_coin_vector balance = {};
	uint64_t native_revision = 0;
	economic_sql_source_digest digest = {};
};
uint16_t native_locator(economic_account_kind kind)
{
	switch (kind)
	{
	case economic_account_kind::wallet:
		return 1;
	case economic_account_kind::bank:
		return 2;
	case economic_account_kind::treasury:
		return 6;
	case economic_account_kind::auction_escrow:
		return 4;
	case economic_account_kind::pending_claim:
		return 5;
	default:
		throw failure{ EINVAL };
	}
}
void frame(std::vector<uint8_t> &output, std::span<const uint8_t> value)
{
	const uint64_t size = value.size();
	for (size_t index = 0; index < 8; ++index)
		output.push_back(static_cast<uint8_t>(size >> (index * 8)));
	output.insert(output.end(), value.begin(), value.end());
}
void number(std::vector<uint8_t> &output, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		output.push_back(static_cast<uint8_t>(value >> (index * 8)));
}
economic_sql_source_digest request_digest(const economic_sql_lifecycle_request &request,
					  bool money_opening = false)
{
	std::vector<uint8_t> data{ 'D', 'U', 'R', 'I', 'S', '-', 'S', 'Q', 'L', '-', 'L',
				   'I', 'F', 'E', 'C', 'Y', 'C', 'L', 'E', '-', 'V', '1' };
	if (money_opening)
		data.back() = '2';
	frame(data, request.operation_id.bytes);
	frame(data, request.lineage.bytes);
	frame(data, request.epoch.bytes);
	number(data, request.actor_id);
	number(data, request.accepted_at_usec);
	return sha(data);
}
// Bind only the complete captured native custody forest. This projection is
// still not authority for physical/world sources missing from the SQL capture.
std::vector<economic_baseline_item>
read_opening_items(const economic_sql_source_snapshot &snapshot,
		   const economic_sql_normalized_sources &normalized)
{
	require(normalized.source_digest == snapshot.digest &&
			normalized.custody_digest == snapshot.custody_digest,
		EILSEQ);
	const auto table = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					[](const auto &value)
					{ return value.name == "item_current_owner"; });
	require(table != snapshot.tables.end() && table->rows.size() == normalized.items.size(),
		EILSEQ);
	require(normalized.items.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES, E2BIG);
	if (normalized.items.empty())
		return {};
	// A default slot from a legacy capture is not evidence of carried custody.
	require(snapshot.version == 2 && snapshot.item_equipment_sources.size() == 1 &&
			normalized.next_uid,
		ENODATA);
	const auto checked_source = [&](const economic_sql_source_reference &source)
	{
		require(source.table < snapshot.tables.size() &&
				source.row < snapshot.tables[source.table].rows.size() &&
				snapshot.tables[source.table].rows[source.row].digest ==
					source.digest,
			EILSEQ);
		return source.digest;
	};
	std::vector<economic_baseline_item> items;
	std::vector<economic_item_snapshot> forest;
	items.reserve(normalized.items.size());
	forest.reserve(normalized.items.size());
	for (const auto &native : normalized.items)
	{
		require(native.source.table ==
					static_cast<size_t>(table - snapshot.tables.begin()) &&
				native.observed_equipment_slot && native.equipment_source &&
				native.owner_revision && native.vnum > 0 &&
				native.item.uid < *normalized.next_uid &&
				native.item.position.root_uid < *normalized.next_uid &&
				native.item.position.parent_uid < *normalized.next_uid &&
				native.item.position.equipment_slot ==
					*native.observed_equipment_slot,
			ENODATA);
		const auto &equipment = *native.equipment_source;
		require(equipment.row == native.source.row &&
				equipment.row < snapshot.item_equipment_sources[0].rows.size() &&
				snapshot.item_equipment_sources[0].rows[equipment.row].digest ==
					equipment.digest,
			EILSEQ);
		const auto owner =
			std::find_if(normalized.owners.begin(), normalized.owners.end(),
				     [&](const auto &value) {
					     return item_owner_identity_equal(
						     value.owner, native.item.position.owner);
				     });
		require(owner != normalized.owners.end() &&
				owner->revision == *native.owner_revision,
			ENODATA);
		std::vector<uint8_t> evidence{ 'E', 'B', 'S', '2' };
		frame(evidence, checked_source(native.source));
		frame(evidence, equipment.digest);
		frame(evidence, checked_source(owner->source));
		items.push_back({ native.item, sha(evidence) });
		forest.push_back(native.item);
	}
	const auto valid = economic_item_effects_validate(forest, forest, {}, 0);
	require(valid == economic_accounting_error::ok,
		valid == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
	return items;
}

economic_sql_source_digest native_digest(const economic_sql_source_snapshot &snapshot,
					 const std::vector<holding_source> &holdings)
{
	const auto wallet = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					 [&](const auto &table)
					 { return table.name == "player_data"; });
	const auto bank = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
				       [&](const auto &table)
				       { return table.name == "account_banks"; });
	const auto shops = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					[&](const auto &table)
					{ return table.name == "shopkeepers"; });
	require(wallet != snapshot.tables.end() && bank != snapshot.tables.end() &&
		shops != snapshot.tables.end());
	std::vector<uint8_t> data{ 'E', 'S', 'N', '1' };
	frame(data, wallet->content_digest);
	frame(data, bank->content_digest);
	std::vector<const holding_source *> piles;
	for (const auto &holding : holdings)
		if (holding.account_kind == economic_account_kind::pile)
			piles.push_back(&holding);
	if (!piles.empty())
	{
		auto pile_data = std::vector<uint8_t>{ 'E', 'S', 'P', '1' };
		number(pile_data, piles.size());
		for (const auto *pile : piles)
		{
			number(pile_data, pile->native_id);
			number(pile_data, pile->native_revision);
			for (const auto amount : pile->balance)
				number(pile_data, static_cast<uint64_t>(amount));
			frame(pile_data, pile->digest);
		}
		data[3] = '2';
		frame(data, sha(pile_data));
	}
	if (!shops->rows.empty())
	{
		data[3] = '3';
		frame(data, shops->content_digest);
	}
	std::vector<const holding_source *> money;
	for (const auto &holding : holdings)
		if (holding.account_kind == economic_account_kind::auction_escrow ||
		    holding.account_kind == economic_account_kind::pending_claim)
			money.push_back(&holding);
	if (!money.empty())
	{
		// Wrap, rather than reinterpret, the exact old ESN1/2/3 preimage.
		std::vector<uint8_t> complete{ 'E', 'S', 'N', '4' };
		frame(complete, sha(data));
		number(complete, money.size());
		for (const auto *holding : money)
		{
			number(complete, static_cast<uint16_t>(holding->account_kind));
			number(complete, holding->native_id);
			number(complete, holding->native_revision);
			for (auto amount : holding->balance)
				number(complete, static_cast<uint64_t>(amount));
			frame(complete, holding->digest);
		}
		data = std::move(complete);
	}
	const auto items = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					[](const auto &table)
					{ return table.name == "item_current_owner"; });
	require(items != snapshot.tables.end(), EILSEQ);
	if (!items->rows.empty())
	{
		// Preserve every original ESN1/2/3/4 preimage; custody uses an explicit
		// successor envelope instead of silently changing an old digest.
		require(snapshot.version == 2, ENODATA);
		std::vector<uint8_t> complete{ 'E', 'S', 'N', '5' };
		frame(complete, sha(data));
		const auto owners = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
						 [](const auto &table)
						 { return table.name == "item_owner_revision"; });
		require(owners != snapshot.tables.end() &&
				snapshot.item_equipment_sources.size() == 1,
			EILSEQ);
		// ESC2 also contains inbox/outbox/mapping rows changed by this same
		// installation. Bind the native item evidence without those receipts.
		frame(complete, items->content_digest);
		frame(complete, owners->content_digest);
		frame(complete, snapshot.item_equipment_sources[0].content_digest);
		frame(complete, snapshot.item_sources_digest);
		return sha(complete);
	}
	return sha(data);
}
std::vector<row> query(MYSQL *connection, const std::string &sql, size_t columns)
{
	execute(connection, sql);
	result_ptr result(mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(result.get()) == columns);
	std::vector<row> rows;
	while (auto raw = mysql_fetch_row(result.get()))
	{
		auto lengths = mysql_fetch_lengths(result.get());
		require(lengths);
		row values;
		values.reserve(columns);
		for (size_t index = 0; index < columns; ++index)
			values.emplace_back(raw[index] ? std::optional<std::string>(std::string(
								 raw[index], lengths[index])) :
							 std::nullopt);
		rows.push_back(std::move(values));
	}
	require(!mysql_errno(connection), mysql_errno(connection));
	return rows;
}
template <typename T> T integer(const std::optional<std::string> &value)
{
	require(value && !value->empty());
	T output = 0;
	const auto parsed = std::from_chars(value->data(), value->data() + value->size(), output);
	require(parsed.ec == std::errc{} && parsed.ptr == value->data() + value->size());
	return output;
}
row one(MYSQL *connection, const std::string &sql, size_t columns)
{
	auto rows = query(connection, sql, columns);
	require(rows.size() == 1, rows.empty() ? ENOENT : EILSEQ);
	return std::move(rows.front());
}
uint64_t scalar(MYSQL *connection, const std::string &sql)
{
	return integer<uint64_t>(one(connection, sql, 1)[0]);
}
void count(MYSQL *connection, const std::string &table, const std::string &where, uint64_t expected)
{
	require(scalar(connection, "SELECT COUNT(*) FROM " + table + " WHERE " + where) ==
		expected);
}
size_t table_index(const economic_sql_source_snapshot &snapshot, std::string_view name)
{
	const auto found = std::find_if(snapshot.tables.begin(), snapshot.tables.end(),
					[&](const auto &table) { return table.name == name; });
	require(found != snapshot.tables.end());
	return static_cast<size_t>(found - snapshot.tables.begin());
}
critical_operation_id parse_id(const std::optional<std::string> &value);
std::vector<holding_source> read_native_holdings(MYSQL *connection,
						 const economic_sql_source_snapshot &snapshot,
						 const economic_sql_normalized_sources &normalized)
{
	const auto wallets = table_index(snapshot, "player_data");
	const auto banks = table_index(snapshot, "account_banks");
	const auto shops = table_index(snapshot, "shopkeepers");
	const auto items = table_index(snapshot, "item_current_owner");
	const auto auctions = table_index(snapshot, "auctions");
	const auto claims = table_index(snapshot, "auction_money_pickups");
	const auto shop_items = std::find_if(snapshot.item_sources.begin(),
					     snapshot.item_sources.end(), [](const auto &source)
					     { return source.name == "shopkeeper_items"; });
	require(shop_items != snapshot.item_sources.end());
	for (const auto &source : shop_items->rows)
		require(source.cells.size() == 6 && source.cells[5], EBUSY);
	std::vector<holding_source> output;
	size_t open_auctions = 0;
	for (const auto &source : snapshot.tables[auctions].rows)
	{
		require(source.cells.size() == 12 && source.cells[2]);
		const auto bidder = integer<int64_t>(source.cells[3]);
		require(bidder >= 0 && static_cast<uint64_t>(bidder) <= UINT32_MAX, ERANGE);
		if (*source.cells[2] == "CLOSED")
			continue;
		if (*source.cells[2] == "REMOVED")
		{
			require(!bidder, EBUSY);
			continue;
		}
		require(*source.cells[2] == "OPEN", EBUSY);
		holding_source native;
		native.account_kind = economic_account_kind::auction_escrow;
		native.native_id = integer<uint32_t>(source.cells[0]);
		require(native.native_id, EILSEQ);
		native.native_revision = integer<uint64_t>(source.cells[7]);
		const auto price = integer<uint64_t>(source.cells[4]);
		require(price <= UINT_MAX, ERANGE);
		// An asking price is not a retained bid. Keep the zero mapping so the
		// original first-bid owner can debit the bidder and credit this escrow.
		native.balance[0] = bidder ? static_cast<int64_t>(price) : 0;
		native.digest = source.digest;
		if (bidder)
		{
			require(price, EILSEQ);
			const auto bid = one(
				connection,
				"SELECT HEX(operation_id),actor_pid,final_price FROM auction_ledger "
				"WHERE auction_id=" +
					std::to_string(native.native_id) +
					" AND auction_revision=" +
					std::to_string(native.native_revision) +
					" AND event_type=2 LOCK IN SHARE MODE",
				3);
			const auto bid_operation = parse_id(bid[0]);
			require(!critical_operation_id_is_zero(bid_operation) &&
					integer<uint32_t>(bid[1]) ==
						static_cast<uint32_t>(bidder) &&
					integer<uint64_t>(bid[2]) == price,
				EILSEQ);
			std::vector<uint8_t> bound{ 'E', 'S', 'A', '1' };
			frame(bound, source.digest);
			frame(bound, bid_operation.bytes);
			number(bound, static_cast<uint64_t>(bidder));
			number(bound, price);
			native.digest = sha(bound);
		}
		output.push_back(std::move(native));
		++open_auctions;
	}
	std::set<std::pair<std::string, uint8_t>> bank_names;
	uint64_t active_coin_rows = 0, unresolved_coin_rows = 0;
	for (const auto &source : snapshot.tables[items].rows)
	{
		require(source.cells.size() == 10);
		if (source.cells[7] && integer<int32_t>(source.cells[7]) == VOBJ_COINS)
		{
			const auto state = integer<uint8_t>(source.cells[8]);
			if (state == static_cast<uint8_t>(item_custody_state::active))
				++active_coin_rows;
			else if (state != static_cast<uint8_t>(item_custody_state::destroyed))
				++unresolved_coin_rows;
		}
	}
	require(unresolved_coin_rows == 0, EBUSY);
	uint64_t selected_coin_rows = 0;
	for (const auto &holding : normalized.holdings)
	{
		if (holding.kind != economic_sql_holding_kind::wallet &&
		    holding.kind != economic_sql_holding_kind::bank &&
		    holding.kind != economic_sql_holding_kind::treasury &&
		    holding.kind != economic_sql_holding_kind::pile &&
		    holding.kind != economic_sql_holding_kind::claim)
			continue;
		require(holding.disposition == economic_sql_holding_disposition::current &&
			holding.balance && holding.native_revision.has_value() &&
			holding.native_id > 0);
		const auto expected_table =
			holding.kind == economic_sql_holding_kind::wallet   ? wallets :
			holding.kind == economic_sql_holding_kind::bank	    ? banks :
			holding.kind == economic_sql_holding_kind::treasury ? shops :
			holding.kind == economic_sql_holding_kind::claim    ? claims :
									      items;
		require(holding.source.row != SIZE_MAX && holding.source.table == expected_table);
		const auto &source = snapshot.tables[holding.source.table].rows[holding.source.row];
		if (holding.kind == economic_sql_holding_kind::treasury)
			require(source.cells.size() == 7 && source.cells[6] &&
					integer<uint8_t>(source.cells[6]) <= 1,
				EBUSY);
		for (auto amount : *holding.balance)
			require(amount >= 0, ERANGE);
		holding_source native;
		native.account_kind = holding.kind == economic_sql_holding_kind::wallet ?
					      economic_account_kind::wallet :
				      holding.kind == economic_sql_holding_kind::bank ?
					      economic_account_kind::bank :
				      holding.kind == economic_sql_holding_kind::treasury ?
					      economic_account_kind::treasury :
				      holding.kind == economic_sql_holding_kind::claim ?
					      economic_account_kind::pending_claim :
					      economic_account_kind::pile;
		native.native_id = holding.native_id;
		native.balance = *holding.balance;
		native.native_revision = *holding.native_revision;
		native.digest = source.digest;
		if (native.account_kind == economic_account_kind::bank)
		{
			require(holding.native_id <= UINT32_MAX && holding.native_context >= 0 &&
					holding.native_context <= INT8_MAX &&
					source.cells.size() == 8,
				EINVAL);
			require(source.cells[1] && !source.cells[1]->empty(), EINVAL);
			native.name = *source.cells[1];
			native.racewar = static_cast<uint8_t>(holding.native_context);
			require(bank_names.emplace(native.name, native.racewar).second, EEXIST);
		}
		else if (native.account_kind == economic_account_kind::wallet)
		{
			require(holding.native_id <= UINT32_MAX && source.cells.size() == 9,
				ERANGE);
		}
		else if (native.account_kind == economic_account_kind::treasury)
		{
			require(holding.native_id <= UINT32_MAX && source.cells.size() == 7 &&
					integer<uint64_t>(source.cells[0]) == holding.native_id &&
					integer<uint64_t>(source.cells[1]) <= UINT32_MAX &&
					(*holding.balance)[1] == 0 && (*holding.balance)[2] == 0 &&
					(*holding.balance)[3] == 0,
				ERANGE);
			native.shop_id = integer<uint32_t>(source.cells[1]);
		}
		else if (native.account_kind == economic_account_kind::pending_claim)
		{
			require(holding.native_id <= UINT32_MAX && source.cells.size() == 3 &&
					(*holding.balance)[0] <= UINT_MAX &&
					!(*holding.balance)[1] && !(*holding.balance)[2] &&
					!(*holding.balance)[3],
				ERANGE);
		}
		else
		{
			require(normalized.next_uid && source.cells.size() == 10 &&
					integer<uint64_t>(source.cells[0]) == holding.native_id &&
					holding.native_id < *normalized.next_uid &&
					integer<int32_t>(source.cells[7]) == VOBJ_COINS &&
					integer<uint8_t>(source.cells[8]) ==
						static_cast<uint8_t>(item_custody_state::active),
				EILSEQ);
			++selected_coin_rows;
		}
		output.push_back(std::move(native));
	}
	std::sort(output.begin(), output.end(),
		  [](const auto &left, const auto &right)
		  {
			  if (left.account_kind != right.account_kind)
				  return left.account_kind < right.account_kind;
			  return left.native_id < right.native_id;
		  });
	const auto wallet_rows = snapshot.tables[wallets].rows.size();
	const auto bank_rows = snapshot.tables[banks].rows.size();
	const auto shop_rows = snapshot.tables[shops].rows.size();
	const auto selected_wallets = static_cast<uint64_t>(
		std::count_if(output.begin(), output.end(), [](const auto &value)
			      { return value.account_kind == economic_account_kind::wallet; }));
	const auto selected_banks = static_cast<uint64_t>(
		std::count_if(output.begin(), output.end(), [](const auto &value)
			      { return value.account_kind == economic_account_kind::bank; }));
	const auto selected_shops = static_cast<uint64_t>(
		std::count_if(output.begin(), output.end(), [](const auto &value)
			      { return value.account_kind == economic_account_kind::treasury; }));
	require(selected_wallets == static_cast<uint64_t>(wallet_rows) &&
			selected_banks == static_cast<uint64_t>(bank_rows) &&
			selected_shops == static_cast<uint64_t>(shop_rows) &&
			selected_coin_rows == active_coin_rows &&
			static_cast<size_t>(
				std::count_if(output.begin(), output.end(),
					      [](const auto &h) {
						      return h.account_kind ==
							     economic_account_kind::pending_claim;
					      })) == snapshot.tables[claims].rows.size() &&
			static_cast<size_t>(
				std::count_if(output.begin(), output.end(),
					      [](const auto &h) {
						      return h.account_kind ==
							     economic_account_kind::auction_escrow;
					      })) == open_auctions,
		EILSEQ);
	return output;
}
void reject_cutover_defects(const economic_sql_normalized_sources &normalized)
{
	using issue = economic_sql_normalization_issue;
	for (const auto kind :
	     { issue::incomplete_receipt, issue::pending_publication, issue::open_quarantine })
		require(normalized.issue_counts[static_cast<size_t>(kind)] == 0, EBUSY);
}
economic_sql_source_digest coverage_digest(const std::vector<holding_source> &holdings,
					   const std::vector<uint64_t> &account_ids)
{
	require(holdings.size() == account_ids.size());
	std::vector<uint8_t> data{ 'E', 'S', 'C', '1' };
	number(data, holdings.size());
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		data.push_back(static_cast<uint8_t>(holdings[index].account_kind));
		number(data, holdings[index].native_id);
		number(data, account_ids[index]);
		data.push_back(holdings[index].racewar);
		frame(data, holdings[index].digest);
	}
	return sha(data);
}
std::string lower_hex(const std::optional<std::string> &value)
{
	require(value && value->size() % 2 == 0);
	std::string output = *value;
	std::transform(output.begin(), output.end(), output.begin(), [](unsigned char character)
		       { return static_cast<char>(std::tolower(character)); });
	return output;
}
critical_operation_id parse_id(const std::optional<std::string> &value)
{
	const auto text = lower_hex(value);
	require(text.size() == 32);
	critical_operation_id output{};
	for (size_t index = 0; index < output.bytes.size(); ++index)
	{
		auto nibble = [](char ch) -> uint8_t
		{
			if (ch >= '0' && ch <= '9')
				return static_cast<uint8_t>(ch - '0');
			if (ch >= 'a' && ch <= 'f')
				return static_cast<uint8_t>(ch - 'a' + 10);
			throw failure{ EILSEQ };
		};
		output.bytes[index] = static_cast<uint8_t>((nibble(text[index * 2]) << 4) |
							   nibble(text[index * 2 + 1]));
	}
	return output;
}
economic_sql_source_digest parse_digest(const std::optional<std::string> &value)
{
	const auto text = lower_hex(value);
	require(text.size() == SHA256_DIGEST_LENGTH * 2);
	economic_sql_source_digest output = {};
	for (size_t index = 0; index < output.size(); ++index)
	{
		auto nibble = [](char ch) -> uint8_t
		{
			if (ch >= '0' && ch <= '9')
				return static_cast<uint8_t>(ch - '0');
			if (ch >= 'a' && ch <= 'f')
				return static_cast<uint8_t>(ch - 'a' + 10);
			throw failure{ EILSEQ };
		};
		output[index] = static_cast<uint8_t>((nibble(text[index * 2]) << 4) |
						     nibble(text[index * 2 + 1]));
	}
	return output;
}
bool digest_nonzero(const economic_sql_source_digest &value)
{
	return std::any_of(value.begin(), value.end(), [](uint8_t byte) { return byte != 0; });
}
void create_operation_receipt(MYSQL *connection, const economic_sql_lifecycle_request &request,
			      const economic_sql_source_digest &request_hash)
{
	const auto empty_keys = sha({});
	const auto command = static_cast<uint16_t>(critical_command_type::economic_baseline);
	execute(connection,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,"
		"schema_version,payload_version,status,result_code,failure_stage,durable_revision,result_payload,committed_at) VALUES(" +
			id(request.operation_id) + "," + digest_sql(request_hash) + "," +
			digest_sql(empty_keys) + "," + std::to_string(command) + "," +
			std::to_string(CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) +
			",1,1,0,0,0,X'',CURRENT_TIMESTAMP(6))");
}
struct stored_installation
{
	bool exists = false;
	critical_operation_id operation = {};
	critical_operation_id lineage = {};
	critical_operation_id epoch = {};
	economic_sql_source_digest request_hash = {};
	economic_sql_source_digest capture_hash = {};
	economic_sql_source_digest native_hash = {};
	std::optional<critical_operation_id> baseline_operation;
	uint64_t wallet_count = 0;
	uint64_t bank_count = 0;
	uint64_t phase = 0;
	std::optional<critical_operation_id> selected_epoch;
	uint64_t revision = 0;
};
stored_installation load_installation(MYSQL *connection, const critical_operation_id &lineage)
{
	const auto rows = query(
		connection,
		"SELECT HEX(operation_id),HEX(lineage),HEX(epoch),HEX(request_digest),"
		"HEX(source_capture_digest),HEX(native_boundary_digest),HEX(baseline_operation_id),"
		"wallet_count,bank_count,phase,HEX(selected_epoch),revision "
		"FROM economic_sql_lifecycle_installation WHERE lineage=" +
			id(lineage) + " FOR UPDATE",
		12);
	require(rows.size() <= 1);
	stored_installation result;
	if (rows.empty())
		return result;
	const auto &record = rows.front();
	result.exists = true;
	result.operation = parse_id(record[0]);
	result.lineage = parse_id(record[1]);
	result.epoch = parse_id(record[2]);
	result.request_hash = parse_digest(record[3]);
	result.capture_hash = parse_digest(record[4]);
	result.native_hash = parse_digest(record[5]);
	if (record[6])
		result.baseline_operation = parse_id(record[6]);
	result.wallet_count = integer<uint64_t>(record[7]);
	result.bank_count = integer<uint64_t>(record[8]);
	result.phase = integer<uint64_t>(record[9]);
	if (record[10])
		result.selected_epoch = parse_id(record[10]);
	result.revision = integer<uint64_t>(record[11]);
	return result;
}
void verify_request(const stored_installation &stored,
		    const economic_sql_lifecycle_request &request,
		    const economic_sql_source_digest &request_hash,
		    const economic_sql_source_digest &native_hash, uint64_t wallets, uint64_t banks)
{
	require(stored.operation.bytes == request.operation_id.bytes &&
			stored.lineage.bytes == request.lineage.bytes &&
			stored.epoch.bytes == request.epoch.bytes &&
			stored.request_hash == request_hash && stored.native_hash == native_hash &&
			stored.wallet_count == wallets && stored.bank_count == banks &&
			stored.phase >= 1 && stored.phase <= 2,
		EEXIST);
}
bool baseline_has_money_opening(const economic_baseline_batch &batch)
{
	return std::any_of(batch.holdings.begin(), batch.holdings.end(),
			   [](const auto &h)
			   {
				   return h.account.kind == economic_account_kind::auction_escrow ||
					  h.account.kind == economic_account_kind::pending_claim;
			   });
}
struct authenticated_opening
{
	economic_sql_lifecycle_request request;
	economic_baseline_batch witness;
	bool money = false;
};
authenticated_opening authenticate_opening(MYSQL *connection, const stored_installation &stored)
{
	require(stored.baseline_operation.has_value(), EILSEQ);
	// The stored plan is bound to the actual original command. Pure preparation
	// leaves different intent/domain digests and cannot be compared directly.
	// Borrow the complete original retained proof under this same transaction.
	const auto baseline_error = economic_sql_baseline_verify_known_retained_in_transaction(
		connection, *stored.baseline_operation, nullptr);
	require(!baseline_error, baseline_error);
	const auto row = one(
		connection,
		"SELECT w.canonical_witness,HEX(w.witness_digest),w.command_accepted_at_usec,"
		"w.claim_origin_version,HEX(i.command_hash),o.canonical_plan "
		"FROM economic_baseline_witness w JOIN critical_operation_inbox i "
		"ON i.operation_id=w.operation_id JOIN economic_accounting_operation o "
		"ON o.operation_id=w.operation_id WHERE w.operation_id=" +
			id(*stored.baseline_operation) + " AND w.lineage=" + id(stored.lineage) +
			" AND w.epoch=" + id(stored.epoch) +
			" AND o.lineage=w.lineage AND o.epoch=w.epoch AND o.outcome=1 AND i.status=1 "
			"AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL "
			"LOCK IN SHARE MODE",
		6);
	require(row[0] && row[5] && row[0]->size() <= ECONOMIC_BASELINE_MAX_BYTES);
	std::optional<economic_prepared_baseline> prepared;
	require(economic_baseline_decode({ reinterpret_cast<const uint8_t *>(row[0]->data()),
					   row[0]->size() },
					 &prepared) == economic_accounting_error::ok,
		EILSEQ);
	require(sha({ reinterpret_cast<const uint8_t *>(row[0]->data()), row[0]->size() }) ==
				parse_digest(row[1]) &&
			prepared->witness().lineage.bytes == stored.lineage.bytes &&
			prepared->witness().epoch.bytes == stored.epoch.bytes &&
			prepared->witness().preparation_id.bytes == stored.operation.bytes &&
			prepared->witness().boundary_digest == stored.native_hash,
		EILSEQ);
	authenticated_opening result;
	result.witness = prepared->witness();
	result.money = baseline_has_money_opening(result.witness);
	result.request.operation_id = stored.operation;
	result.request.lineage = stored.lineage;
	result.request.epoch = stored.epoch;
	result.request.actor_id = result.witness.actor_id;
	const auto receipt = one(
		connection,
		"SELECT HEX(command_hash),HEX(keys_hash),command_type,schema_version,payload_version,"
		"status,result_code,failure_stage,durable_revision,OCTET_LENGTH(result_payload),"
		"committed_at IS NOT NULL FROM critical_operation_inbox WHERE operation_id=" +
			id(stored.operation) + " LOCK IN SHARE MODE",
		11);
	require(parse_digest(receipt[0]) == stored.request_hash &&
			parse_digest(receipt[1]) == sha({}) &&
			integer<uint16_t>(receipt[2]) ==
				static_cast<uint16_t>(critical_command_type::economic_baseline) &&
			integer<uint16_t>(receipt[3]) ==
				CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			integer<uint16_t>(receipt[4]) == 1 && integer<uint16_t>(receipt[5]) == 1 &&
			!integer<uint64_t>(receipt[6]) && !integer<uint64_t>(receipt[7]) &&
			!integer<uint64_t>(receipt[8]) && !integer<uint64_t>(receipt[9]) &&
			integer<uint16_t>(receipt[10]) == 1,
		EILSEQ);
	if (row[2])
	{
		result.request.accepted_at_usec = integer<uint64_t>(row[2]);
		require(result.request.actor_id && result.request.accepted_at_usec, EILSEQ);
		critical_command original;
		require(economic_baseline_command_build(*prepared, result.request.accepted_at_usec,
							&original) ==
					economic_accounting_error::ok &&
				original.operation_id.bytes == stored.baseline_operation->bytes,
			EILSEQ);
		std::vector<uint8_t> encoded;
		require(critical_command_encode(original, &encoded) ==
					critical_command_codec_result::ok &&
				sha(encoded) == parse_digest(row[4]) &&
				request_digest(result.request, result.money) == stored.request_hash,
			EILSEQ);
	}
	else
	{
		// Genuine historical V1 may lack its original timestamp. No clock,
		// creation timestamp or supplied caller preimage repairs that absence.
		require(!result.money && !row[3], ENOTSUP);
	}
	if (result.money)
		require(row[3] && integer<uint16_t>(row[3]) == 1 && result.request.accepted_at_usec,
			ENODATA);
	else if (row[3])
		require(integer<uint16_t>(row[3]) == 1, EILSEQ);
	if (row[3])
	{
		const auto origin_error = economic_sql_pending_claim_source_verify_baseline(
			connection, *stored.baseline_operation, result.witness);
		require(!origin_error, origin_error);
	}
	return result;
}

std::vector<uint64_t> create_or_verify_mappings(MYSQL *connection,
						const economic_sql_lifecycle_request &request,
						const std::vector<holding_source> &holdings,
						bool create)
{
	const auto mapped_count = static_cast<size_t>(
		std::count_if(holdings.begin(), holdings.end(), [](const auto &holding)
			      { return holding.account_kind != economic_account_kind::pile; }));
	if (create)
	{
		std::set<uint64_t> pile_ids;
		for (const auto &holding : holdings)
			if (holding.account_kind == economic_account_kind::pile)
				pile_ids.insert(holding.native_id);
		auto next_mapping_id =
			scalar(connection,
			       "SELECT COALESCE(MAX(mapping_id),0) FROM economic_account_mapping");
		require(next_mapping_id < std::numeric_limits<uint64_t>::max(), EOVERFLOW);
		++next_mapping_id;
		for (const auto &holding : holdings)
		{
			if (holding.account_kind == economic_account_kind::pile)
				continue;
			while (pile_ids.contains(next_mapping_id))
			{
				require(next_mapping_id < std::numeric_limits<uint64_t>::max(),
					EOVERFLOW);
				++next_mapping_id;
			}
			const uint16_t account_kind = static_cast<uint16_t>(holding.account_kind);
			const bool bank = holding.account_kind == economic_account_kind::bank;
			const uint16_t locator = native_locator(holding.account_kind);
			execute(connection,
				"INSERT INTO economic_account_mapping(mapping_id,lineage,account_kind,context_id,backend_kind,"
				"locator_kind,native_id,active_native_id,creating_operation_id,retiring_operation_id,revision) VALUES(" +
					std::to_string(next_mapping_id) + "," +
					id(request.lineage) + "," + std::to_string(account_kind) +
					"," + std::to_string(bank ? holding.racewar : 0) + ",1," +
					std::to_string(locator) + "," +
					std::to_string(holding.native_id) + "," +
					std::to_string(holding.native_id) + "," +
					id(request.operation_id) + ",NULL,0)");
			require(next_mapping_id < std::numeric_limits<uint64_t>::max(), EOVERFLOW);
			++next_mapping_id;
		}
	}
	const auto rows = query(
		connection,
		"SELECT mapping_id,account_kind,context_id,backend_kind,locator_kind,native_id,"
		"active_native_id,HEX(creating_operation_id),HEX(retiring_operation_id),revision "
		"FROM economic_account_mapping WHERE lineage=" +
			id(request.lineage) + " ORDER BY locator_kind,native_id FOR UPDATE",
		10);
	require(rows.size() == mapped_count, EILSEQ);
	std::vector<uint64_t> account_ids(holdings.size());
	size_t mapping_index = 0;
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		const auto &source = holdings[index];
		if (source.account_kind == economic_account_kind::pile)
		{
			account_ids[index] = source.native_id;
			continue;
		}
		const auto &record = rows[mapping_index++];
		const auto account_kind = integer<uint16_t>(record[1]);
		const auto context = integer<uint64_t>(record[2]);
		const auto locator = integer<uint16_t>(record[4]);
		const bool bank = source.account_kind == economic_account_kind::bank;
		const auto expected_kind = static_cast<uint16_t>(source.account_kind);
		const uint16_t expected_locator = native_locator(source.account_kind);
		require(account_kind == expected_kind && context == (bank ? source.racewar : 0) &&
				integer<uint8_t>(record[3]) == ECONOMIC_MAPPING_BACKEND_SQL &&
				locator == expected_locator &&
				integer<uint64_t>(record[5]) == source.native_id &&
				integer<uint64_t>(record[6]) == source.native_id &&
				parse_id(record[7]).bytes == request.operation_id.bytes &&
				!record[8] && integer<uint64_t>(record[9]) == 0,
			EILSEQ);
		const auto mapping = integer<uint64_t>(record[0]);
		require(mapping > 0, EILSEQ);
		account_ids[index] = mapping;
	}
	return account_ids;
}
void verify_retired_escrow(MYSQL *connection, const stored_installation &stored, uint64_t native,
			   const critical_operation_id &retiring, std::span<const uint8_t> account)
{
	const auto auction = std::to_string(native);
	const auto terminal = one(
		connection,
		"SELECT l.auction_revision,a.auction_revision,l.event_type,a.status,"
		"a.winning_bidder_pid,a.seller_pid,HEX(a.listing_operation_id) "
		"FROM economic_accounting_operation o JOIN critical_operation_inbox i "
		"ON i.operation_id=o.operation_id JOIN economic_accounting_source_claim claim "
		"ON claim.operation_id=o.operation_id AND claim.lineage=o.lineage "
		"AND claim.source_event=o.source_event AND claim.outcome=1 "
		"JOIN economic_accounting_account_effect e "
		"ON e.operation_id=o.operation_id JOIN auctions a ON a.id=" +
			auction +
			" JOIN auction_ledger l ON l.auction_id=a.id AND l.operation_id=o.operation_id "
			"WHERE o.operation_id=" +
			id(retiring) + " AND o.lineage=" + id(stored.lineage) +
			" AND o.epoch=" + id(stored.epoch) +
			" AND o.outcome=1 AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 "
			"AND i.committed_at IS NOT NULL AND e.account_key=" +
			binary(account) +
			" AND e.after_copper=0 AND e.after_silver=0 AND e.after_gold=0 AND e.after_platinum=0 "
			"AND a.custody_state=1 AND l.event_type IN (3,4,7) LOCK IN SHARE MODE",
		7);
	auto revision = integer<uint64_t>(terminal[0]);
	const auto current = integer<uint64_t>(terminal[1]);
	const auto type = integer<uint16_t>(terminal[2]);
	const auto winner = integer<uint32_t>(terminal[4]);
	const auto seller = integer<uint32_t>(terminal[5]);
	require(revision && revision <= current && seller && terminal[3] &&
			((type == 3 && *terminal[3] == "CLOSED" && winner) ||
			 (type == 4 && *terminal[3] == "CLOSED" && !winner) ||
			 (type == 7 && *terminal[3] == "REMOVED")),
		EILSEQ);
	count(connection, "auction_ledger", "auction_id=" + auction + " AND event_type IN (3,4,7)",
	      1);
	// Retirement stays at its original sale/expiry/removal revision. Native
	// item collection alone may then advance the auction, once per staged
	// item subset. Authenticate every contiguous successor; <= is not proof.
	const auto successors = query(
		connection,
		"SELECT HEX(l.operation_id),l.event_type,l.auction_revision,l.actor_pid,l.item_count,"
		"o.canonical_plan,HEX(o.plan_digest),o.canonical_intent,i.durable_revision,i.result_payload "
		"FROM auction_ledger l LEFT JOIN economic_accounting_operation o ON o.operation_id=l.operation_id "
		"LEFT JOIN critical_operation_inbox i ON i.operation_id=l.operation_id WHERE l.auction_id=" +
			auction + " AND l.auction_revision>" + std::to_string(revision) +
			" ORDER BY l.auction_revision,l.operation_id LOCK IN SHARE MODE",
		10);
	require(successors.size() <= AUCTION_COMMAND_MAX_ITEMS, EILSEQ);
	const auto claimant = type == 3 ? winner : seller;
	for (const auto &record : successors)
	{
		require(revision < UINT64_MAX && integer<uint16_t>(record[1]) == 6 &&
				integer<uint64_t>(record[2]) == revision + 1 &&
				integer<uint32_t>(record[3]) == claimant && record[5] &&
				record[7] && record[9] &&
				record[9]->size() == AUCTION_RESULT_PAYLOAD_BYTES,
			EILSEQ);
		const auto operation = parse_id(record[0]);
		const auto items = integer<uint16_t>(record[4]);
		require(items && items <= AUCTION_COMMAND_MAX_ITEMS &&
				!critical_operation_id_is_zero(operation),
			EILSEQ);
		// The native root retains the maximum of all five returned revisions,
		// not just the auction's contiguous revision. Decode the complete original
		// receipt and compare its exact canonical bytes, including unused tail.
		auction_command_result result{};
		std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> canonical_result{};
		require(auction_command_decode_result(
				reinterpret_cast<const uint8_t *>(record[9]->data()),
				record[9]->size(), &result) &&
				auction_command_encode_result(result, &canonical_result) &&
				std::memcmp(canonical_result.data(), record[9]->data(),
					    canonical_result.size()) == 0 &&
				result.action == auction_action::claim_item &&
				result.event_type == auction_event_type::item_claimed &&
				result.auction_id == native &&
				result.auction_revision == revision + 1 &&
				result.status == (type == 7 ? 3U : 2U) &&
				result.seller_pid == seller && result.winner_pid == claimant &&
				result.item_count == items &&
				integer<uint64_t>(record[8]) ==
					std::max({ result.auction_revision, result.wallet_revision,
						   result.bank_revision,
						   result.player_owner_revision,
						   result.auction_owner_revision }),
			EILSEQ);
		economic_accounting_plan plan;
		require(economic_plan_decode({ reinterpret_cast<const uint8_t *>(record[5]->data()),
					       record[5]->size() },
					     &plan) == economic_accounting_error::ok &&
				sha({ reinterpret_cast<const uint8_t *>(record[5]->data()),
				      record[5]->size() }) == parse_digest(record[6]) &&
				record[7]->size() <= CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES,
			EILSEQ);
		economic_frozen_intent intent;
		economic_digest intent_digest = {};
		require(economic_intent_decode(
				{ reinterpret_cast<const uint8_t *>(record[7]->data()),
				  record[7]->size() },
				&intent) == economic_accounting_error::ok &&
				economic_intent_digest(intent, &intent_digest) ==
					economic_accounting_error::ok &&
				intent_digest == plan.metadata.intent_digest &&
				intent.domain_digest == plan.metadata.domain_digest,
			EILSEQ);
		const auto &meta = plan.metadata;
		const auto listing = parse_id(terminal[6]);
		require(meta.operation_id.bytes == operation.bytes &&
				meta.lineage.bytes == stored.lineage.bytes &&
				meta.epoch.bytes == stored.epoch.bytes &&
				meta.actor_kind == economic_actor_kind::domain &&
				meta.actor_id == claimant &&
				meta.writer_id == ECONOMIC_WRITER_AUCTION_ITEM_CLAIM &&
				meta.reason == economic_reason::auction_claim &&
				meta.original_operation_id.bytes == listing.bytes &&
				meta.source_event &&
				meta.source_event->kind == economic_source_kind::auction &&
				meta.source_event->source.bytes == retiring.bytes &&
				meta.source_event->generation.bytes == listing.bytes &&
				meta.source_event->sequence == revision &&
				!meta.source_event->slot && plan.accounts.empty() &&
				plan.postings.empty() && plan.children.empty() &&
				plan.item_events.size() == items &&
				plan.items_before.size() == items &&
				plan.items_after.size() == items,
			EILSEQ);
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source = {};
		require(economic_source_event_encode(*meta.source_event, &source) ==
				economic_accounting_error::ok,
			EILSEQ);
		const auto claim_status = economic_sql_auction_source_claim_verify_known_metadata(
			connection, meta, 0);
		require(!claim_status, claim_status);
		count(connection,
		      "economic_accounting_operation o JOIN critical_operation_inbox i ON i.operation_id=o.operation_id",
		      "o.operation_id=" + id(operation) + " AND o.lineage=" + id(stored.lineage) +
			      " AND o.epoch=" + id(stored.epoch) +
			      " AND o.outcome=1 AND o.writer_id=" +
			      std::to_string(ECONOMIC_WRITER_AUCTION_ITEM_CLAIM) +
			      " AND o.reason=" +
			      std::to_string(
				      static_cast<uint16_t>(economic_reason::auction_claim)) +
			      " AND o.actor_id=" + std::to_string(claimant) +
			      " AND o.original_operation_id=" + id(listing) +
			      " AND o.source_event=" + binary(source) +
			      " AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL",
		      1);
		count(connection, "auction_item_custody",
		      "auction_id=" + auction + " AND claim_operation_id=" + id(operation) +
			      " AND claim_pid=" + std::to_string(claimant) +
			      " AND claimed_at IS NOT NULL",
		      items);
		count(connection, "economic_accounting_item_reference",
		      "operation_id=" + id(operation), items);
		count(connection, "item_ownership_ledger", "operation_id=" + id(operation), items);
		for (const auto &event : plan.item_events)
		{
			require(!event.child_index && event.uid &&
					event.event_index < result.item_count &&
					result.item_uids[event.event_index] == event.uid &&
					result.item_revisions[event.event_index] ==
						event.after.revision &&
					event.before.owner.type == item_owner_type::auction &&
					event.before.owner.id == native &&
					!event.before.owner.context_id &&
					event.before.root_uid == event.uid &&
					!event.before.parent_uid &&
					event.before.state == item_custody_state::active &&
					event.before.revision < UINT64_MAX &&
					event.after.owner.type == item_owner_type::player &&
					event.after.owner.id == claimant &&
					!event.after.owner.context_id &&
					event.after.root_uid == event.uid &&
					!event.after.parent_uid &&
					event.after.state == item_custody_state::active &&
					event.after.revision == event.before.revision + 1 &&
					!event.before.equipment_slot && !event.after.equipment_slot,
				EILSEQ);
			count(connection,
			      "auction_item_custody c JOIN economic_accounting_item_reference r ON r.item_uid=c.item_uid "
			      "JOIN item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND l.event_index=r.legacy_event_index "
			      "AND l.item_uid=r.item_uid AND l.item_revision=r.after_revision",
			      "c.auction_id=" + auction +
				      " AND c.item_uid=" + std::to_string(event.uid) +
				      " AND c.claim_operation_id=" + id(operation) +
				      " AND c.claim_pid=" + std::to_string(claimant) +
				      " AND c.claimed_at IS NOT NULL AND c.item_revision=" +
				      std::to_string(event.after.revision) +
				      " AND r.operation_id=" + id(operation) +
				      " AND r.legacy_operation_id=" + id(operation) +
				      " AND r.event_index=" + std::to_string(event.event_index) +
				      " AND r.legacy_event_index=" +
				      std::to_string(event.event_index) +
				      " AND r.before_revision=" +
				      std::to_string(event.before.revision) +
				      " AND r.after_revision=" +
				      std::to_string(event.after.revision) +
				      " AND l.from_owner_type=6 AND l.from_owner_id=" + auction +
				      " AND l.from_owner_context_id=0 "
				      "AND l.to_owner_type=1 AND l.to_owner_id=" +
				      std::to_string(claimant) + " AND l.to_owner_context_id=0",
			      1);
		}
		++revision;
	}
	require(revision == current, EILSEQ);
}

std::vector<uint64_t> verify_current_mappings(
	MYSQL *connection, const stored_installation &stored, const authenticated_opening &opening,
	const std::vector<holding_source> &holdings,
	std::vector<economic_sql_native_mobile_wallet_lifetime> *retained_native = nullptr)
{
	// Authenticate birth inboxes before mapping/current-native locks. These
	// wallets form a separate lifetime namespace and never enter PID exports.
	std::vector<economic_sql_native_mobile_wallet_lifetime> native_wallets;
	const auto native_error = economic_sql_native_mobile_birth_lock_wallet_lifetimes(
		connection, stored.lineage, &native_wallets);
	require(!native_error, native_error);
	std::map<uint64_t, const economic_sql_native_mobile_wallet_lifetime *> native_lifetimes;
	for (const auto &wallet : native_wallets)
	{
		require(wallet.account.lineage.bytes == stored.lineage.bytes &&
				wallet.account.kind == economic_account_kind::wallet &&
				wallet.account.context_id ==
					ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT &&
				wallet.account.authority_id && wallet.native_id &&
				wallet.native_revision &&
				wallet.native_state == quest_mobile_lifetime_state::live &&
				!critical_operation_id_is_zero(wallet.creating_operation_id) &&
				!critical_operation_id_is_zero(wallet.birth_epoch) &&
				critical_operation_id_is_zero(wallet.retiring_operation_id) &&
				wallet.active_native_id == wallet.native_id &&
				wallet.mapping_revision == 0,
			EILSEQ);
		require(native_lifetimes.emplace(wallet.account.authority_id, &wallet).second,
			EEXIST);
	}
	const auto rows = query(
		connection,
		"SELECT mapping_id,account_kind,context_id,backend_kind,locator_kind,native_id,"
		"active_native_id,HEX(creating_operation_id),HEX(retiring_operation_id),revision "
		"FROM economic_account_mapping WHERE lineage=" +
			id(stored.lineage) + " ORDER BY mapping_id FOR UPDATE",
		10);
	using locator_key = std::tuple<economic_account_kind, uint16_t, uint64_t, uint64_t>;
	std::map<locator_key, uint64_t> live;
	std::map<uint64_t, uint64_t> contexts;
	for (const auto &record : rows)
	{
		const auto mapping = integer<uint64_t>(record[0]);
		const auto kind = static_cast<economic_account_kind>(integer<uint16_t>(record[1]));
		const auto context = integer<uint64_t>(record[2]);
		const auto native = integer<uint64_t>(record[5]);
		const auto creating = parse_id(record[7]);
		const auto revision = integer<uint64_t>(record[9]);
		const auto locator = integer<uint16_t>(record[4]);
		const auto native_found = native_lifetimes.find(mapping);
		const bool native_namespace = kind == economic_account_kind::wallet &&
					      (locator == ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR ||
					       context == ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT);
		if (native_namespace || native_found != native_lifetimes.end())
		{
			require(native_found != native_lifetimes.end(), EILSEQ);
			const auto &wallet = *native_found->second;
			require(kind == economic_account_kind::wallet &&
					integer<uint16_t>(record[3]) ==
						ECONOMIC_MAPPING_BACKEND_SQL &&
					locator == ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR &&
					context == wallet.account.context_id &&
					native == wallet.native_id &&
					creating.bytes == wallet.creating_operation_id.bytes &&
					!record[8] && record[6] &&
					integer<uint64_t>(record[6]) == wallet.active_native_id &&
					revision == wallet.mapping_revision,
				EILSEQ);
			native_lifetimes.erase(native_found);
			continue;
		}
		require(mapping && native &&
				integer<uint16_t>(record[3]) == ECONOMIC_MAPPING_BACKEND_SQL &&
				integer<uint16_t>(record[4]) == native_locator(kind) &&
				!critical_operation_id_is_zero(creating),
			EILSEQ);
		require(kind == economic_account_kind::bank ? context <= INT8_MAX : context == 0,
			EILSEQ);
		const bool original = creating.bytes == stored.operation.bytes;
		const bool money = kind == economic_account_kind::auction_escrow ||
				   kind == economic_account_kind::pending_claim;
		const bool retired = record[8].has_value();
		economic_account_key account{ stored.lineage, kind, mapping, context };
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
		require(economic_account_key_encode(account, &encoded) ==
			economic_accounting_error::ok);
		const auto original_holding =
			std::find_if(opening.witness.holdings.begin(),
				     opening.witness.holdings.end(), [&](const auto &h)
				     { return economic_account_key_equal(h.account, account); });
		if (original)
			require(original_holding != opening.witness.holdings.end(), EILSEQ);
		if (!money)
		{
			require(original && !retired && revision == 0 && record[6] &&
					integer<uint64_t>(record[6]) == native,
				EILSEQ);
		}
		else
		{
			require(context == 0 && native <= UINT32_MAX, EILSEQ);
			if (!original)
			{
				bool zero_created_pending = false;
				if (kind == economic_account_kind::pending_claim)
				{
					const auto creator_effect = query(
						connection,
						"SELECT e.before_copper,e.before_silver,e.before_gold,e.before_platinum,"
						"e.after_copper,e.after_silver,e.after_gold,e.after_platinum,"
						"e.before_revision,e.after_revision FROM economic_accounting_operation o "
						"JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
						"JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id "
						"WHERE o.operation_id=" +
							id(creating) +
							" AND o.lineage=" + id(stored.lineage) +
							" AND o.epoch=" + id(stored.epoch) +
							" AND o.outcome=1 AND i.status=1 AND i.result_code=0 "
							"AND i.failure_stage=0 AND i.committed_at IS NOT NULL AND e.account_key=" +
							binary(encoded),
						10);
					require(creator_effect.size() == 1, EILSEQ);
					zero_created_pending = true;
					for (size_t column = 0; column < 9; ++column)
						zero_created_pending =
							zero_created_pending &&
							integer<uint64_t>(
								creator_effect[0][column]) == 0;
					zero_created_pending =
						zero_created_pending &&
						integer<uint64_t>(creator_effect[0][9]) == 1;
					if (zero_created_pending)
						require(economic_sql_pending_claim_endpoint_verify_zero_creator(
								connection, creating, account,
								static_cast<uint32_t>(native)) == 0,
							EILSEQ);
				}
				if (!zero_created_pending)
					count(connection,
					      "economic_accounting_operation o JOIN critical_operation_inbox i "
					      "ON i.operation_id=o.operation_id JOIN economic_accounting_account_effect e "
					      "ON e.operation_id=o.operation_id",
					      "o.operation_id=" + id(creating) +
						      " AND o.lineage=" + id(stored.lineage) +
						      " AND o.epoch=" + id(stored.epoch) +
						      " AND o.outcome=1 AND i.status=1 "
						      "AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL "
						      "AND e.account_key=" +
						      binary(encoded) +
						      (kind == economic_account_kind::pending_claim ?
							       " AND e.after_copper>e.before_copper AND e.before_silver=e.after_silver "
							       "AND e.before_gold=e.after_gold AND e.before_platinum=e.after_platinum" :
							       " AND e.before_copper=0 AND e.after_copper=0 AND e.before_silver=0 "
							       "AND e.after_silver=0 AND e.before_gold=0 AND e.after_gold=0 "
							       "AND e.before_platinum=0 AND e.after_platinum=0"),
					      1);
				if (kind == economic_account_kind::auction_escrow)
					count(connection,
					      "auctions a JOIN auction_ledger l ON l.auction_id=a.id",
					      "a.id=" + std::to_string(native) +
						      " AND a.listing_operation_id=" +
						      id(creating) + " AND l.operation_id=" +
						      id(creating) + " AND l.event_type=1",
					      1);
				else if (!zero_created_pending)
					count(connection, "economic_pending_claim_source",
					      "source_operation_id=" + id(creating) +
						      " AND lineage=" + id(stored.lineage) +
						      " AND claim_mapping_id=" +
						      std::to_string(mapping) +
						      " AND beneficiary_pid=" +
						      std::to_string(native),
					      1);
			}
			if (retired)
			{
				// Only the existing auction settlement/buyout owners retire
				// these money lifetimes. Pending claim rows keep their mapping.
				require(kind == economic_account_kind::auction_escrow &&
						!record[6] && revision == 1,
					EILSEQ);
				const auto retiring = parse_id(record[8]);
				require(!critical_operation_id_is_zero(retiring), EILSEQ);
				verify_retired_escrow(connection, stored, native, retiring,
						      encoded);
				continue;
			}
			require(record[6] && integer<uint64_t>(record[6]) == native &&
					revision == 0,
				EILSEQ);
		}
		require(live.emplace(locator_key{ kind, native_locator(kind), context, native },
				     mapping)
				.second,
			EEXIST);
		contexts.emplace(mapping, context);
	}
	require(native_lifetimes.empty(), EILSEQ);
	std::vector<uint64_t> ids;
	ids.reserve(holdings.size());
	for (const auto &holding : holdings)
	{
		if (holding.account_kind == economic_account_kind::pile)
		{
			ids.push_back(holding.native_id);
			continue;
		}
		const auto found = live.find(
			{ holding.account_kind, native_locator(holding.account_kind),
			  holding.account_kind == economic_account_kind::bank ? holding.racewar :
										uint64_t{ 0 },
			  holding.native_id });
		require(found != live.end(), EILSEQ);
		require(contexts.at(found->second) ==
				(holding.account_kind == economic_account_kind::bank ?
					 holding.racewar :
					 0),
			EILSEQ);
		ids.push_back(found->second);
		// Bind retained claim allocations to native current money at cold boot.
		if (holding.account_kind == economic_account_kind::pending_claim)
		{
			economic_account_key account{ stored.lineage, holding.account_kind,
						      found->second, 0 };
			std::vector<economic_sql_pending_claim_remaining> sources;
			const auto error = economic_sql_pending_claim_source_remaining(
				connection, account, static_cast<uint32_t>(holding.native_id),
				&sources);
			require(!error, error);
			uint64_t total = 0;
			for (const auto &source : sources)
			{
				require(source.remaining_amount <= UINT_MAX - total, EILSEQ);
				total += source.remaining_amount;
			}
			require(total == static_cast<uint64_t>(holding.balance[0]), EILSEQ);
		}
		live.erase(found);
	}
	require(live.empty(), EILSEQ);
	// Move the genuine current lifetime proof only after every mapping passes.
	// Earlier callers keep their original behavior and no lifetime is re-queried.
	if (retained_native)
		*retained_native = std::move(native_wallets);
	return ids;
}

economic_baseline_batch make_batch(const economic_sql_lifecycle_request &request,
				   const std::vector<holding_source> &holdings,
				   const std::vector<uint64_t> &account_ids,
				   const economic_sql_source_digest &native_hash,
				   const std::vector<economic_baseline_item> &items)
{
	require(holdings.size() == account_ids.size());
	economic_baseline_batch batch;
	batch.lineage = request.lineage;
	batch.epoch = request.epoch;
	batch.preparation_id = request.operation_id;
	batch.actor_id = request.actor_id;
	batch.batch_index = 0;
	batch.opening_account = { request.lineage, economic_account_kind::opening, 1, 0 };
	batch.boundary_digest = native_hash;
	batch.coverage_digest = coverage_digest(holdings, account_ids);
	batch.items = items;
	if (!items.empty())
	{
		std::vector<uint8_t> complete{ 'E', 'I', 'C', '2' };
		frame(complete, batch.coverage_digest);
		number(complete, items.size());
		for (const auto &item : items)
		{
			number(complete, item.snapshot.uid);
			frame(complete, item.source_digest);
		}
		batch.coverage_digest = sha(complete);
	}
	batch.holdings.reserve(holdings.size());
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		const auto &source = holdings[index];
		economic_account_key account{
			request.lineage, source.account_kind, account_ids[index],
			static_cast<uint64_t>(source.account_kind == economic_account_kind::bank ?
						      source.racewar :
						      0)
		};
		batch.holdings.push_back(
			{ account, source.balance, source.native_revision, source.digest });
	}
	return batch;
}
critical_operation_id baseline_id(const economic_sql_lifecycle_request &request)
{
	critical_operation_id value{};
	require(critical_operation_id_derive(request.operation_id,
					     ECONOMIC_BASELINE_OPERATION_DOMAIN, 0, &value),
		EINVAL);
	return value;
}
using baseline_initializer = unsigned int (*)(MYSQL *, const critical_operation_id &,
					      const critical_operation_id &,
					      const economic_account_key &,
					      const critical_operation_id &);
void create_installation(MYSQL *connection, const economic_sql_lifecycle_request &request,
			 const economic_sql_source_digest &request_hash,
			 const economic_sql_source_digest &capture_hash,
			 const economic_sql_source_digest &native_hash, uint64_t wallets,
			 uint64_t banks, const std::vector<holding_source> &holdings,
			 baseline_initializer initialize)
{
	count(connection, "economic_lineage_state", "lineage=" + id(request.lineage), 0);
	count(connection, "economic_account_mapping", "lineage=" + id(request.lineage), 0);
	count(connection, "critical_operation_inbox", "operation_id=" + id(request.operation_id),
	      0);
	create_operation_receipt(connection, request, request_hash);
	execute(connection,
		"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(" +
			id(request.lineage) + ",NULL,0)");
	execute(connection,
		"INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,transition_digest,creating_operation_id) VALUES(" +
			id(request.lineage) + "," + id(request.epoch) + ",1,NULL,1," +
			digest_sql(native_hash) + "," + id(request.operation_id) + ")");
	(void)create_or_verify_mappings(connection, request, holdings, true);
	const economic_account_key opening{ request.lineage, economic_account_kind::opening, 1, 0 };
	const auto initialized = initialize(connection, request.lineage, request.epoch, opening,
					    request.operation_id);
	require(initialized == 0, initialized);
	execute(connection,
		"INSERT INTO economic_sql_lifecycle_installation(operation_id,lineage,epoch,request_digest,"
		"source_capture_digest,native_boundary_digest,baseline_operation_id,wallet_count,bank_count,phase,selected_epoch,revision) VALUES(" +
			id(request.operation_id) + "," + id(request.lineage) + "," +
			id(request.epoch) + "," + digest_sql(request_hash) + "," +
			digest_sql(capture_hash) + "," + digest_sql(native_hash) + ",NULL," +
			std::to_string(wallets) + "," + std::to_string(banks) + ",1,NULL,0)");
}
void ensure_no_preexisting_mapping(const economic_sql_source_snapshot &snapshot,
				   const critical_operation_id &lineage)
{
	const auto index = table_index(snapshot, "economic_account_mapping");
	for (const auto &record : snapshot.tables[index].rows)
	{
		require(record.cells.size() == 11);
		if (record.cells[1] && record.cells[1]->size() == lineage.bytes.size() &&
		    std::equal(record.cells[1]->begin(), record.cells[1]->end(),
			       reinterpret_cast<const char *>(lineage.bytes.data())))
			throw failure{ EEXIST };
	}
}

void fill_export(const economic_sql_lifecycle_request &request,
		 const std::vector<holding_source> &holdings,
		 const std::vector<uint64_t> &account_ids, const stored_installation &stored,
		 uint64_t revision, economic_sql_lifecycle_receipt *output)
{
	economic_sql_lifecycle_receipt receipt;
	receipt.operation_id = request.operation_id;
	receipt.lineage = request.lineage;
	receipt.epoch = request.epoch;
	receipt.baseline_operation_id = stored.baseline_operation.value_or(baseline_id(request));
	receipt.source_capture_digest = stored.capture_hash;
	receipt.native_boundary_digest = stored.native_hash;
	receipt.baseline_revision = revision;
	for (size_t index = 0; index < holdings.size(); ++index)
	{
		const auto &source = holdings[index];
		if (source.account_kind == economic_account_kind::pile)
			continue;
		economic_account_key account{
			request.lineage, source.account_kind, account_ids[index],
			static_cast<uint64_t>(source.account_kind == economic_account_kind::bank ?
						      source.racewar :
						      0)
		};
		if (source.account_kind == economic_account_kind::bank)
			receipt.banks.push_back({ source.name, source.racewar, account });
		else if (source.account_kind == economic_account_kind::treasury)
			receipt.treasuries.push_back({ source.shop_id,
						       static_cast<uint32_t>(source.native_id),
						       account });
		else if (source.account_kind == economic_account_kind::wallet)
			receipt.wallets.push_back(
				{ static_cast<uint32_t>(source.native_id), account });
	}
	*output = std::move(receipt);
}
void verify_staged_epoch(MYSQL *connection, const economic_sql_lifecycle_request &request,
			 const critical_operation_id &baseline_operation, uint64_t revision)
{
	const auto lineage =
		one(connection,
		    "SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" +
			    id(request.lineage) + " LOCK IN SHARE MODE",
		    1);
	require(!lineage[0], EPERM);
	const auto row =
		one(connection,
		    "SELECT phase,HEX(selected_epoch),HEX(baseline_operation_id),revision FROM "
		    "economic_sql_lifecycle_installation WHERE operation_id=" +
			    id(request.operation_id) + " LOCK IN SHARE MODE",
		    4);
	require(integer<uint8_t>(row[0]) == 2 && parse_id(row[1]).bytes == request.epoch.bytes &&
			parse_id(row[2]).bytes == baseline_operation.bytes &&
			integer<uint64_t>(row[3]) == 1 && revision == 1,
		EILSEQ);
}
void select_staged_epoch(MYSQL *connection, const economic_sql_lifecycle_request &request,
			 const economic_sql_source_digest &request_hash,
			 const critical_operation_id &baseline_operation)
{
	execute(connection,
		"UPDATE economic_sql_lifecycle_installation SET baseline_operation_id=" +
			id(baseline_operation) +
			",phase=2,selected_epoch=epoch,revision=revision+1 WHERE operation_id=" +
			id(request.operation_id) + " AND lineage=" + id(request.lineage) +
			" AND epoch=" + id(request.epoch) +
			" AND request_digest=" + digest_sql(request_hash) +
			" AND phase=1 AND revision=0 AND selected_epoch IS NULL");
	require(mysql_affected_rows(connection) == 1);
}
void check_active_epoch_null(MYSQL *connection, const critical_operation_id &lineage)
{
	const auto state = one(
		connection,
		"SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" + id(lineage),
		1);
	require(!state[0], EPERM);
}
struct stored_activation
{
	bool exists = false;
	critical_operation_id epoch = {};
	critical_operation_id installation = {};
	critical_operation_id baseline = {};
	economic_sql_source_digest manifest_digest = {};
	economic_sql_source_digest audit_digest = {};
	uint64_t route_count = 0;
	uint64_t verified_route_count = 0;
	uint64_t unclassified_route_count = 0;
	uint64_t state = 0;
	uint64_t revision = 0;
};
stored_activation load_activation(MYSQL *connection, const critical_operation_id &lineage)
{
	const auto rows =
		query(connection,
		      "SELECT HEX(epoch),HEX(installation_operation_id),HEX(baseline_operation_id),"
		      "HEX(manifest_digest),HEX(audit_digest),route_count,"
		      "verified_route_count,unclassified_route_count,state,revision FROM "
		      "economic_sql_global_activation WHERE lineage=" +
			      id(lineage) + " FOR UPDATE",
		      10);
	require(rows.size() <= 1);
	stored_activation output;
	if (rows.empty())
		return output;
	const auto &record = rows.front();
	output.exists = true;
	output.epoch = parse_id(record[0]);
	output.installation = parse_id(record[1]);
	output.baseline = parse_id(record[2]);
	output.manifest_digest = parse_digest(record[3]);
	output.audit_digest = parse_digest(record[4]);
	output.route_count = integer<uint64_t>(record[5]);
	output.verified_route_count = integer<uint64_t>(record[6]);
	output.unclassified_route_count = integer<uint64_t>(record[7]);
	output.state = integer<uint64_t>(record[8]);
	output.revision = integer<uint64_t>(record[9]);
	return output;
}
void verify_baseline_receipt(MYSQL *connection, const stored_installation &stored,
			     std::optional<uint64_t> expected_holdings = std::nullopt)
{
	require(stored.baseline_operation && stored.phase == 2 && stored.revision == 1 &&
			stored.selected_epoch && stored.selected_epoch->bytes == stored.epoch.bytes,
		EILSEQ);
	const auto receipt = one(
		connection,
		"SELECT HEX(c.last_operation_id),c.revision,w.holding_count,o.outcome,"
		"i.status FROM economic_baseline_control c JOIN economic_baseline_witness w "
		"ON w.lineage=c.lineage AND w.epoch=c.epoch AND w.operation_id=c.last_operation_id "
		"JOIN economic_accounting_operation o ON o.operation_id=w.operation_id "
		"JOIN critical_operation_inbox i ON i.operation_id=w.operation_id "
		"WHERE c.lineage=" +
			id(stored.lineage) + " AND c.epoch=" + id(stored.epoch) +
			" LOCK IN SHARE MODE",
		5);
	const auto holding_count = integer<uint64_t>(receipt[2]);
	require(parse_id(receipt[0]).bytes == stored.baseline_operation->bytes &&
			integer<uint64_t>(receipt[1]) == 1 &&
			holding_count >= stored.wallet_count + stored.bank_count &&
			(!expected_holdings || holding_count == *expected_holdings) &&
			integer<uint64_t>(receipt[3]) == 1 && integer<uint64_t>(receipt[4]) == 1,
		EILSEQ);
}
// This is a persisted correspondence prerequisite, never a complete live/world
// census or original provenance admission. Consume all rows/counts, not capped
// diagnostic details. The independent verifier and original authority stay separate.
void reject_persisted_correspondence_defects(const economic_sql_persisted_correspondence &report)
{
	require(report.multiply_matched_active_custody_indices.empty(), EILSEQ);
	require(report.unmatched_active_custody_indices.empty(), ENODATA);
	using physical_issue = economic_sql_physical_issue;
	for (size_t index = 0; index < report.physical.issue_counts.size(); ++index)
	{
		const auto kind = static_cast<physical_issue>(index);
		// The eight-source report cannot count room/auction/collector matches.
		// Only the complete union above can resolve this provider-local finding.
		if (kind == physical_issue::unmatched_active_custody)
			continue;
		const bool incomplete = kind == physical_issue::unresolved_owner ||
					kind == physical_issue::missing_uid ||
					kind == physical_issue::unresolved_parent ||
					kind == physical_issue::unknown_equipment ||
					kind == physical_issue::unmatched_physical;
		require(!report.physical.issue_counts[index], incomplete ? ENODATA : EILSEQ);
	}
	constexpr uint32_t room_classification = SQL_ROOM_SOURCE_CURRENT | SQL_ROOM_SOURCE_HISTORY |
						 SQL_ROOM_SOURCE_OLD_SEASON |
						 SQL_ROOM_SOURCE_MOVED_FROM_ROOM;
	require(report.room.season_active && report.room.current_season, ENODATA);
	require(!report.room.native_root_limit_exceeded, E2BIG);
	// Retain the original drop-reader evidence. Only independently proven
	// creation literals can replace its provider-local false classification.
	require(report.creation_observed, ENODATA);
	require(!report.creation.global_flags, ENODATA);
	require(report.creation.season_active &&
			report.creation.current_season == report.room.current_season,
		ENODATA);
	require(!report.creation.native_root_limit_exceeded, E2BIG);
	for (const auto &family : report.creation.families)
		require(!family.flags, ENODATA);
	std::set<size_t> superseded;
	for (const auto index : report.creation.superseded_room_witness_indices)
		require(index < report.room.witnesses.size() && superseded.insert(index).second,
			EILSEQ);
	std::set<std::tuple<uint64_t, uint64_t, uint64_t>> creation_graphs;
	for (const auto &graph : report.creation.graphs)
	{
		require(graph.valid && !graph.witness_indices.empty(), ENODATA);
		require(creation_graphs
				.emplace(graph.root_item_uid, graph.room, graph.owner_revision)
				.second,
			EILSEQ);
	}
	for (const auto &witness : report.creation.witnesses)
		require(!(witness.room.flags & ~room_classification), ENODATA);
	for (size_t index = 0; index < report.room.witnesses.size(); ++index)
		if (!superseded.count(index))
			require(!(report.room.witnesses[index].flags & ~room_classification),
				ENODATA);
	for (const auto &graph : report.room.graphs)
	{
		const bool creation_replaces =
			!graph.witness_indices.empty() &&
			std::all_of(graph.witness_indices.begin(), graph.witness_indices.end(),
				    [&](size_t index) { return superseded.count(index) != 0; }) &&
			creation_graphs.count(
				{ graph.root_item_uid, graph.room, graph.owner_revision });
		require(graph.valid || creation_replaces, EILSEQ);
	}

	using auction_issue = auction_physical_issue;
	for (size_t index = 0; index < report.auction.issue_counts.size(); ++index)
	{
		const auto kind = static_cast<auction_issue>(index);
		// Unknown legacy/original origins are not physical contradictions or a
		// new accounting-command admission requirement for pre-accounting data.
		// Ambiguous decoder refusal is scoped to current slots below.
		if (kind == auction_issue::original_provenance_unknown ||
		    kind == auction_issue::opaque_literal ||
		    kind == auction_issue::unbound_listing ||
		    kind == auction_issue::generic_proof_unknown ||
		    kind == auction_issue::literal_decode_refused)
			continue;
		require(!report.auction.issue_counts[index],
			(kind == auction_issue::missing_literal ||
			 kind == auction_issue::missing_identity) ?
				ENODATA :
			kind == auction_issue::native_bound_refused ? E2BIG :
								      EILSEQ);
	}
	require(report.auction.malformed_rows.empty() && report.auction.extra_custody.empty(),
		EILSEQ);
	require(report.auction.unmatched_auction_custody.empty() &&
			report.auction.missing_literal_events.empty(),
		ENODATA);
	// Bound work by the captured rows, rather than rescanning every identity
	// for every slot. The source ceilings already bound both vector lengths.
	std::vector<size_t> legacy_matches(report.auction.slots.size());
	for (const auto &witness : report.auction.legacy_identities)
	{
		require(witness.slot_index < legacy_matches.size(), EILSEQ);
		if (witness.current_field_correspondence)
			++legacy_matches[witness.slot_index];
	}
	for (size_t slot_index = 0; slot_index < report.auction.slots.size(); ++slot_index)
	{
		const auto &slot = report.auction.slots[slot_index];
		require(slot.claimed.has_value(), ENODATA);
		if (*slot.claimed)
			continue; // Claimed retained revisions are historical, never occupancy.
		if (!slot.node_count)
		{
			// A v1 native receipt can prove singleton custody without decoded
			// object properties. Count only the distinct, exact union provider;
			// opaque bytes and original origin remain unknown, not repaired.
			require(legacy_matches[slot_index] == 1, ENODATA);
			continue;
		}
		require(slot.node_count && slot.node_begin <= report.auction.nodes.size() &&
				slot.node_count <= report.auction.nodes.size() - slot.node_begin,
			ENODATA);
		for (size_t offset = 0; offset < slot.node_count; ++offset)
			require(report.auction.nodes[slot.node_begin + offset]
					.current_field_correspondence,
				ENODATA);
	}
	for (const auto &pickup : report.auction.pickups)
		require(pickup.retrieved && *pickup.retrieved == 1, ENODATA);
	// Retrieved opaque legacy pickups stay history; no UID is invented from text.
	// ANF2/EAI/EAP and retained_command_proof_known remain unknown in these reports.
	require(report.collector.catalog_valid && report.collector.correspondence_valid, ENODATA);
	for (const auto &listing : report.collector.listings)
		require(!listing.findings && listing.correspondence_valid, ENODATA);
	for (const auto &death : report.collector.deaths)
		require(!death.findings, ENODATA);
	for (const auto &custody : report.collector.custody)
		require(!custody.findings, ENODATA);
}

// Join only the real native participant captured above in this same RR cut.
// Its borrowed references address the immutable source2 rows; no historical
// origin, cash-unknown value or source2 wire/hash contract is manufactured.
void join_current_native_catalog(const economic_sql_physical_source_snapshot &base,
				 economic_sql_persisted_correspondence &report,
				 quest_mobile_native_sql_catalog &&catalog)
{
	require(catalog.physical_digest == base.digest, EILSEQ);
	for (size_t index = 0; index < catalog.issue_counts.size(); ++index)
	{
		// Historical v1 unknown cash alone is not missing item identity.
		if ((uint64_t{ 1 } << index) == quest_mobile_native_catalog_flags::cash_unknown)
			continue;
		require(!catalog.issue_counts[index], ENODATA);
	}
	require(catalog.extra_custody.empty() && catalog.unmatched_active_custody.empty() &&
			catalog.malformed_custody.empty(),
		ENODATA);
	const auto &items = report.physical.source2.items;
	require(report.custody.size() == items.size(), EILSEQ);
	std::map<size_t, size_t> by_raw_row;
	for (size_t index = 0; index < items.size(); ++index)
		require(items[index].source.table == 11 &&
				by_raw_row.emplace(items[index].source.row, index).second &&
				report.custody[index].custody_index == index,
			EILSEQ);
	for (size_t index = 0; index < catalog.items.size(); ++index)
	{
		const auto &witness = catalog.items[index];
		require(witness.current_field_correspondence && witness.custody &&
				witness.catalog_row < catalog.catalog.size(),
			ENODATA);
		const auto &native = catalog.catalog[witness.catalog_row];
		require(native.image && native.image->state == quest_mobile_lifetime_state::live &&
				witness.image_item < native.image->items.size(),
			EILSEQ);
		const auto &literal = native.image->items[witness.image_item];
		const auto found = by_raw_row.find(witness.custody->row);
		require(found != by_raw_row.end() &&
				witness.custody->row < base.source2.tables[11].rows.size() &&
				witness.custody->digest ==
					base.source2.tables[11].rows[witness.custody->row].digest,
			EILSEQ);
		const auto &item = items[found->second].item;
		require(item.uid == literal.object_uid &&
				item.position.state == item_custody_state::active &&
				item.position.owner.type == item_owner_type::native_mobile &&
				item.position.owner.id ==
					native.image->reference.mobile_instance_id &&
				item.position.owner.context_id == 0,
			EILSEQ);
		report.custody[found->second].matches.push_back(
			{ economic_sql_persisted_provider::native_mobile_literal, index });
	}
	// Retain every raw/history row and all original provider-local diagnostics.
	// Recompute only the complete union disposition after adding native matches.
	report.native_mobile = std::move(catalog);
	report.unmatched_active_custody_indices.clear();
	report.multiply_matched_active_custody_indices.clear();
	for (size_t index = 0; index < items.size(); ++index)
		if (items[index].item.position.state == item_custody_state::active)
		{
			if (report.custody[index].matches.empty())
				report.unmatched_active_custody_indices.push_back(index);
			else if (report.custody[index].matches.size() > 1)
				report.multiply_matched_active_custody_indices.push_back(index);
		}
}

// This checks the complete current cache against this same persisted cut.
// The serialized maintenance owner excludes writers throughout capture; real
// current owner clocks are observed separately from older item-entry clocks.
// Missing cache for offline/unloaded durable holdings is not absence evidence.
// This is a prerequisite only: complete initialized-world/literal/cash and
// original release qualification remain with their respective owners.
void reject_runtime_cache_correspondence(const economic_sql_persisted_correspondence &persisted,
					 const economic_sql_source_limits &limits)
{
	std::vector<item_ownership_runtime_entry> cache;
	const auto captured = item_ownership_runtime_snapshot_all_active(
		static_cast<size_t>(limits.maximum_rows), &cache);
	require(!captured, captured);
	std::vector<std::optional<uint64_t>> clocks;
	clocks.reserve(cache.size());
	for (const auto &entry : cache)
	{
		uint64_t revision = 0;
		if (item_ownership_runtime_peek_owner_revision(entry.owner, &revision))
			clocks.emplace_back(revision);
		else
			clocks.emplace_back(std::nullopt);
	}
	economic_sql_runtime_cache_correspondence report;
	const auto compared =
		economic_sql_compare_runtime_cache(cache, clocks, persisted, limits, 512, &report);
	require(compared == economic_accounting_error::ok,
		compared == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
	// All findings count; truncating stored detail cannot clear a mismatch.
	require(report.findings.empty() &&
			std::all_of(report.issue_counts.begin(), report.issue_counts.end(),
				    [](uint64_t count) { return count == 0; }),
		ENODATA);
}

struct complete_activation_capture
{
	economic_sql_physical_source_snapshot physical;
	sql_room_item_source_snapshot room;
	sql_room_creation_source_snapshot creation;
	economic_sql_persisted_correspondence correspondence;
	zone_reset_creation_budget_totals totals;
	std::vector<holding_source> holdings;
	std::vector<economic_sql_native_mobile_wallet_lifetime> native_wallets;
};
// Own the complete existing SQL cut beside its indexed reports. It never starts
// or ends the caller's transaction and does not acquire world/cutover authority.
complete_activation_capture
capture_complete_activation_sources(MYSQL *connection, const economic_sql_source_limits &limits)
{
	complete_activation_capture complete;
	const auto session = mysql_thread_id(connection);
	auto &base = complete.physical;
	auto captured =
		economic_sql_capture_physical_sources_in_transaction(connection, limits, &base);
	require(!captured, captured);
	auto &supplement = complete.room;
	captured = sql_room_item_payload_capture_sources_in_transaction(connection, limits, base,
									&supplement, 512);
	require(!captured, captured);
	auto &creation = complete.creation;
	captured = sql_room_creation_source_capture_in_transaction(connection, limits, base,
								   supplement, &creation);
	require(!captured, captured);
	auto &report = complete.correspondence;
	const auto status = economic_sql_normalize_persisted_correspondence(
		base, supplement, creation, limits, 512, &report);
	require(status == economic_accounting_error::ok,
		status == economic_accounting_error::capacity	? ENOMEM :
		status == economic_accounting_error::unresolved ? ENODATA :
								  EILSEQ);
	economic_sql_source_limits native_limits;
	const auto budget_prepared = zone_reset_creation_budget_prepare(base, supplement, creation,
									limits, &native_limits);
	require(!budget_prepared, budget_prepared);
	quest_mobile_native_sql_catalog native;
	const auto native_captured = quest_mobile_native_sql_capture_catalog_in_transaction(
		connection, base, supplement, native_limits, &native);
	require(!native_captured, native_captured);
	require(native.original_session == session, ENOTCONN);
	auto &combined = complete.totals;
	const auto budget_validated = zone_reset_creation_budget_validate_native(
		base, supplement, creation, limits, native, &combined);
	require(!budget_validated, budget_validated);
	join_current_native_catalog(base, report, std::move(native));
	reject_cutover_defects(report.physical.source2);
	reject_persisted_correspondence_defects(report);
	reject_runtime_cache_correspondence(report, limits);
	require(mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS),
		ENOTCONN);
	complete.holdings = read_native_holdings(connection, base.source2, report.physical.source2);
	require(mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS),
		ENOTCONN);
	// Keep the original verifier input/hash/baseline contracts. EPH1/room
	// hashes bind this one cut only; they must never replace ESN1-5 or ESC2.
	return complete;
}
std::pair<economic_sql_source_snapshot, std::vector<holding_source>>
capture_current_holdings(MYSQL *connection, bool reject_defects)
{
	if (reject_defects)
	{
		auto captured = capture_complete_activation_sources(connection, {});
		// Preserve the established source2/hash/holdings contract for old callers.
		return { std::move(captured.physical.source2), std::move(captured.holdings) };
	}
	economic_sql_source_snapshot snapshot;
	const auto captured =
		economic_sql_capture_sources_in_transaction(connection, {}, &snapshot);
	require(captured == 0, captured);
	economic_sql_normalized_sources normalized;
	const auto status = economic_sql_normalize_sources(snapshot, 512, &normalized);
	require(status == economic_accounting_error::ok,
		status == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
	if (reject_defects)
		reject_cutover_defects(normalized);
	auto holdings = read_native_holdings(connection, snapshot, normalized);
	return { std::move(snapshot), std::move(holdings) };
}
struct transaction
{
	MYSQL *connection;
	unsigned long session;
	bool started = false;
	~transaction()
	{
		if (started && mysql_thread_id(connection) == session)
			(void)mysql_real_query(connection, "ROLLBACK", 8);
	}
};
// Shared original selection/opening proof; transaction and authority publication
// remain with the lifecycle class owner. No current mapping/native proof here.
struct verified_runtime_selection
{
	critical_operation_id lineage, epoch;
	stored_installation installation;
	authenticated_opening opening;
};
std::optional<verified_runtime_selection> load_verified_runtime_selection(MYSQL *connection)
{
	const auto active_rows =
		query(connection,
		      "SELECT HEX(lineage),HEX(active_epoch) FROM economic_lineage_state "
		      "WHERE active_epoch IS NOT NULL FOR UPDATE",
		      2);
	require(active_rows.size() <= 1, EILSEQ);
	if (active_rows.empty())
		return std::nullopt;
	const auto lineage = parse_id(active_rows[0][0]);
	const auto epoch = parse_id(active_rows[0][1]);
	auto stored = load_installation(connection, lineage);
	const auto activation = load_activation(connection, lineage);
	require(stored.exists && stored.phase == 2 && stored.epoch.bytes == epoch.bytes &&
			activation.exists && activation.state == 1 &&
			activation.epoch.bytes == epoch.bytes &&
			activation.installation.bytes == stored.operation.bytes &&
			stored.baseline_operation &&
			activation.baseline.bytes == stored.baseline_operation->bytes &&
			activation.route_count > 0 &&
			activation.verified_route_count == activation.route_count &&
			activation.unclassified_route_count == 0 &&
			digest_nonzero(activation.manifest_digest) &&
			digest_nonzero(activation.audit_digest),
		EILSEQ);
	verify_baseline_receipt(connection, stored);
	auto opening = authenticate_opening(connection, stored);
	return verified_runtime_selection{ lineage, epoch, std::move(stored), std::move(opening) };
}
economic_sql_lifecycle_receipt
load_verified_runtime_projection(MYSQL *connection, const verified_runtime_selection &selected)
{
	const auto &lineage = selected.lineage;
	const auto &epoch = selected.epoch;
	const auto &stored = selected.installation;
	const auto &opening = selected.opening;
	auto [snapshot, holdings] = capture_current_holdings(connection, false);
	(void)snapshot;
	require(stored.wallet_count == static_cast<uint64_t>(std::count_if(
					       holdings.begin(), holdings.end(),
					       [](const auto &value) {
						       return value.account_kind ==
							      economic_account_kind::wallet;
					       })) &&
			stored.bank_count == static_cast<uint64_t>(std::count_if(
						     holdings.begin(), holdings.end(),
						     [](const auto &value) {
							     return value.account_kind ==
								    economic_account_kind::bank;
						     })),
		EILSEQ);
	economic_sql_lifecycle_request request;
	request.operation_id = stored.operation;
	request.lineage = lineage;
	request.epoch = epoch;
	const auto account_ids = verify_current_mappings(connection, stored, opening, holdings);
	economic_sql_lifecycle_receipt receipt;
	fill_export(request, holdings, account_ids, stored, 1, &receipt);
	return receipt;
}
} // namespace

unsigned int economic_sql_accounting_lifecycle_transaction::install(
	MYSQL *connection, const economic_sql_lifecycle_guard &authority,
	const economic_sql_lifecycle_request &request,
	economic_sql_lifecycle_receipt *output) noexcept
{
	try
	{
		require(connection && output, EINVAL);
		require(authority.is_maintenance_authority() &&
				authority.connection_ == connection &&
				authority.session_ == mysql_thread_id(connection),
			EPERM);
		require(!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
				(connection->server_status & SERVER_STATUS_AUTOCOMMIT),
			EBUSY);
		require(!critical_operation_id_is_zero(request.operation_id) &&
				!critical_operation_id_is_zero(request.lineage) &&
				!critical_operation_id_is_zero(request.epoch) && request.actor_id &&
				request.accepted_at_usec &&
				request.operation_id.bytes != request.lineage.bytes &&
				request.operation_id.bytes != request.epoch.bytes &&
				request.lineage.bytes != request.epoch.bytes,
			EINVAL);
		const auto contract_error = economic_sql_pending_claim_source_contract(connection);
		require(!contract_error, contract_error);
		// Raw death-conflict archives are never opening holdings. Until a
		// separately audited resolution path exists, every case remains open.
		// Maintenance owns the writer fence also used by the retention writer.
		require(scalar(connection, "SELECT COUNT(*) FROM player_death_conflict_evidence") ==
				0,
			EBUSY);
		execute(connection, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");
		// Maintenance retains its writer fence across capture and installation.
		// Borrow the complete persisted prerequisite in one owned RR transaction.
		// READ WRITE permits the original holdings reader's shared locks; this
		// capture executes only reads and rolls back before any install writes.
		// The public owning READ ONLY source2 reader remains unchanged.
		auto [snapshot, holdings] = [&]()
		{
			execute(connection, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");
			const auto session = mysql_thread_id(connection);
			// Arm cleanup before START: its acknowledgement may be lost.
			transaction capture{ connection, session, true };
			execute(connection,
				"START TRANSACTION WITH CONSISTENT SNAPSHOT, READ WRITE");
			auto cut = capture_current_holdings(connection, true);
			require(mysql_thread_id(connection) == session &&
					(connection->server_status & SERVER_STATUS_IN_TRANS),
				ENOTCONN);
			execute(connection, "ROLLBACK");
			capture.started = false;
			require(mysql_thread_id(connection) == session &&
					!(connection->server_status & SERVER_STATUS_IN_TRANS),
				ENOTCONN);
			return cut;
		}();
		economic_sql_normalized_sources normalized;
		const auto normalized_result =
			economic_sql_normalize_sources(snapshot, 512, &normalized);
		require(normalized_result == economic_accounting_error::ok,
			normalized_result == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
		reject_cutover_defects(normalized);
		const auto opening_items = read_opening_items(snapshot, normalized);
		const bool money_opening = std::any_of(
			holdings.begin(), holdings.end(),
			[](const auto &h)
			{
				return h.account_kind == economic_account_kind::auction_escrow ||
				       h.account_kind == economic_account_kind::pending_claim;
			});
		const auto expected_request_hash = request_digest(request, money_opening);
		const auto native_hash = native_digest(snapshot, holdings);
		const auto capture_hash = snapshot.digest;
		const auto wallets = static_cast<uint64_t>(std::count_if(
			holdings.begin(), holdings.end(), [](const auto &value)
			{ return value.account_kind == economic_account_kind::wallet; }));
		const auto banks = static_cast<uint64_t>(std::count_if(
			holdings.begin(), holdings.end(), [](const auto &value)
			{ return value.account_kind == economic_account_kind::bank; }));
		require(wallets + banks <= holdings.size() &&
				holdings.size() <= ECONOMIC_BASELINE_MAX_HOLDINGS,
			E2BIG);
		const auto existing = load_installation(connection, request.lineage);
		std::vector<uint64_t> account_ids;
		if (!existing.exists)
		{
			ensure_no_preexisting_mapping(snapshot, request.lineage);
			const auto session = mysql_thread_id(connection);
			transaction owner{ connection, session, true };
			execute(connection, "START TRANSACTION");
			const auto initialize_baseline = [](MYSQL *database,
							    const critical_operation_id &lineage,
							    const critical_operation_id &epoch,
							    const economic_account_key &opening,
							    const critical_operation_id &operation)
			{
				return economic_sql_baseline_transaction::initialize(
					database, lineage, epoch, opening, operation);
			};
			create_installation(connection, request, expected_request_hash,
					    capture_hash, native_hash, wallets, banks, holdings,
					    initialize_baseline);
			require(mysql_thread_id(connection) == session &&
					(connection->server_status & SERVER_STATUS_IN_TRANS),
				ENOTCONN);
			execute(connection, "COMMIT");
			owner.started = false;
			account_ids =
				create_or_verify_mappings(connection, request, holdings, false);
		}
		else
		{
			verify_request(existing, request, expected_request_hash, native_hash,
				       wallets, banks);
			account_ids =
				create_or_verify_mappings(connection, request, holdings, false);
		}
		const auto stored = load_installation(connection, request.lineage);
		require(stored.exists);
		verify_request(stored, request, expected_request_hash, native_hash, wallets, banks);
		auto batch = make_batch(request, holdings, account_ids, native_hash, opening_items);
		std::optional<economic_prepared_baseline> prepared;
		require(economic_baseline_prepare(batch, &prepared) ==
				economic_accounting_error::ok,
			EINVAL);
		critical_command command{};
		require(economic_baseline_command_build(*prepared, request.accepted_at_usec,
							&command) == economic_accounting_error::ok,
			EINVAL);
		const auto expected_baseline_id = baseline_id(request);
		require(command.operation_id.bytes == expected_baseline_id.bytes, EILSEQ);
		critical_apply_result applied =
			economic_sql_baseline_transaction::apply(connection, command, *prepared);
		if (applied.outcome == critical_apply_outcome::ambiguous_commit ||
		    applied.outcome == critical_apply_outcome::retryable_failure)
		{
			const auto reconciled =
				economic_sql_baseline_transaction::reconcile(connection, command);
			if (reconciled.outcome != critical_apply_outcome::already_applied)
				throw failure{ reconciled.error_code ? reconciled.error_code :
					       applied.error_code    ? applied.error_code :
								       EAGAIN };
			applied = reconciled;
		}
		require(applied.outcome == critical_apply_outcome::applied ||
				applied.outcome == critical_apply_outcome::already_applied,
			applied.error_code ? applied.error_code : EIO);
		const auto receipt =
			economic_sql_baseline_transaction::reconcile(connection, command);
		require(receipt.outcome == critical_apply_outcome::already_applied &&
				receipt.durable_revision == applied.durable_revision,
			EILSEQ);
		const auto session = mysql_thread_id(connection);
		transaction selection{ connection, session, true };
		execute(connection, "START TRANSACTION");
		if (stored.phase == 1)
			select_staged_epoch(connection, request, expected_request_hash,
					    expected_baseline_id);
		else
			require(stored.phase == 2 && stored.baseline_operation &&
					stored.baseline_operation->bytes ==
						expected_baseline_id.bytes &&
					stored.selected_epoch &&
					stored.selected_epoch->bytes == request.epoch.bytes &&
					stored.revision == 1,
				EILSEQ);
		check_active_epoch_null(connection, request.lineage);
		if (stored.phase == 1)
			verify_staged_epoch(connection, request, expected_baseline_id,
					    receipt.durable_revision);
		(void)authenticate_opening(connection,
					   load_installation(connection, request.lineage));
		require(mysql_thread_id(connection) == selection.session &&
				(connection->server_status & SERVER_STATUS_IN_TRANS),
			ENOTCONN);
		execute(connection, "COMMIT");
		selection.started = false;
		fill_export(request, holdings, account_ids, stored, receipt.durable_revision,
			    output);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

unsigned int economic_sql_accounting_lifecycle_transaction::activate_verified(
	MYSQL *connection, economic_sql_cutover_transaction_owner &owner,
	const economic_sql_lifecycle_request &request,
	const economic_sql_activation_evidence &evidence,
	economic_sql_activation_verifier verify) noexcept
{
	try
	{
		require(connection, EINVAL);
		require(owner.is_valid() && owner.connection_ == connection && owner.maintenance_ &&
				owner.writer_lock_ && mysql_thread_id(connection) == owner.session_,
			EPERM);
		require(verify && evidence.route_count &&
				evidence.verified_route_count == evidence.route_count &&
				!evidence.unclassified_route_count &&
				digest_nonzero(evidence.manifest_digest) &&
				digest_nonzero(evidence.audit_digest),
			ENODATA);
		require(!critical_operation_id_is_zero(request.operation_id) &&
				!critical_operation_id_is_zero(request.lineage) &&
				!critical_operation_id_is_zero(request.epoch),
			EINVAL);
		// A committed active decision is an exact retry even after gameplay has
		// changed native holdings. Its durable evidence and pointer must still
		// match; a paused decision must pass fresh capture and verification below.
		const auto existing = load_activation(connection, request.lineage);
		if (existing.exists && existing.state == 1)
		{
			const auto staged = load_installation(connection, request.lineage);
			require(staged.exists && staged.phase == 2 && staged.baseline_operation &&
					staged.operation.bytes == request.operation_id.bytes &&
					staged.epoch.bytes == request.epoch.bytes &&
					staged.request_hash ==
						request_digest(request, authenticate_opening(
										connection, staged)
										.money) &&
					existing.epoch.bytes == request.epoch.bytes &&
					existing.installation.bytes == request.operation_id.bytes &&
					existing.baseline.bytes ==
						staged.baseline_operation->bytes &&
					existing.manifest_digest == evidence.manifest_digest &&
					existing.audit_digest == evidence.audit_digest &&
					existing.route_count == evidence.route_count &&
					existing.verified_route_count ==
						evidence.verified_route_count &&
					existing.unclassified_route_count == 0,
				EEXIST);
			const auto selected = one(
				connection,
				"SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" +
					id(request.lineage) + " FOR UPDATE",
				1);
			require(selected[0] && parse_id(selected[0]).bytes == request.epoch.bytes,
				EILSEQ);
			return activate(connection, owner, request.lineage);
		}
		auto [snapshot, holdings] = capture_current_holdings(connection, true);
		execute(connection, "SAVEPOINT economic_sql_activation_verifier");
		const auto verified = verify(connection, request, evidence, snapshot);
		require(!verified, verified);
		execute(connection, "ROLLBACK TO SAVEPOINT economic_sql_activation_verifier");
		execute(connection, "RELEASE SAVEPOINT economic_sql_activation_verifier");
		require(owner.is_valid(), EPERM);
		const auto stored = load_installation(connection, request.lineage);
		require(stored.exists && stored.operation.bytes == request.operation_id.bytes &&
				stored.epoch.bytes == request.epoch.bytes &&
				stored.request_hash ==
					request_digest(
						request,
						authenticate_opening(connection, stored).money) &&
				stored.wallet_count ==
					static_cast<uint64_t>(std::count_if(
						holdings.begin(), holdings.end(),
						[](const auto &value) {
							return value.account_kind ==
							       economic_account_kind::wallet;
						})) &&
				stored.bank_count ==
					static_cast<uint64_t>(std::count_if(
						holdings.begin(), holdings.end(),
						[](const auto &value) {
							return value.account_kind ==
							       economic_account_kind::bank;
						})),
			EILSEQ);
		const auto activation = load_activation(connection, request.lineage);
		if (activation.exists)
		{
			const auto opening = authenticate_opening(connection, stored);
			verify_baseline_receipt(connection, stored);
			(void)verify_current_mappings(connection, stored, opening, holdings);
		}
		else
		{
			verify_baseline_receipt(connection, stored, holdings.size());
			(void)create_or_verify_mappings(connection, request, holdings, false);
		}
		const auto lineage =
			one(connection,
			    "SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" +
				    id(request.lineage) + " FOR UPDATE",
			    1);
		if (!activation.exists)
		{
			require(!lineage[0] &&
					stored.native_hash == native_digest(snapshot, holdings),
				ESTALE);
			execute(connection,
				"INSERT INTO economic_sql_global_activation(lineage,epoch,"
				"installation_operation_id,baseline_operation_id,manifest_digest,"
				"audit_digest,route_count,verified_route_count,"
				"unclassified_route_count,state,revision) VALUES(" +
					id(request.lineage) + "," + id(request.epoch) + "," +
					id(request.operation_id) + "," +
					id(*stored.baseline_operation) + "," +
					digest_sql(evidence.manifest_digest) + "," +
					digest_sql(evidence.audit_digest) + "," +
					std::to_string(evidence.route_count) + "," +
					std::to_string(evidence.verified_route_count) + ",0,1,1)");
		}
		else
		{
			require(activation.epoch.bytes == request.epoch.bytes &&
					activation.installation.bytes ==
						request.operation_id.bytes &&
					activation.baseline.bytes ==
						stored.baseline_operation->bytes &&
					activation.manifest_digest == evidence.manifest_digest &&
					activation.audit_digest == evidence.audit_digest &&
					activation.route_count == evidence.route_count &&
					activation.verified_route_count ==
						evidence.verified_route_count &&
					activation.unclassified_route_count == 0,
				EEXIST);
			require((activation.state == 1 && lineage[0] &&
				 parse_id(lineage[0]).bytes == request.epoch.bytes) ||
					(activation.state == 2 && !lineage[0]),
				EILSEQ);
			if (activation.state == 2)
			{
				execute(connection,
					"UPDATE economic_sql_global_activation SET state=1,revision=revision+1 "
					"WHERE lineage=" +
						id(request.lineage) + " AND state=2 AND revision=" +
						std::to_string(activation.revision));
				require(mysql_affected_rows(connection) == 1, ESTALE);
			}
		}
		return activate(connection, owner, request.lineage);
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

unsigned int economic_sql_accounting_lifecycle_transaction::activate(
	MYSQL *connection, economic_sql_cutover_transaction_owner &owner,
	const critical_operation_id &lineage, uint64_t *new_lineage_revision) noexcept
{
	try
	{
		require(connection, EINVAL);
		require(!critical_operation_id_is_zero(lineage), EINVAL);
		require(owner.is_valid() && owner.connection_ == connection && owner.maintenance_ &&
				owner.writer_lock_ && mysql_thread_id(connection) == owner.session_,
			EPERM);
		const auto stored = load_installation(connection, lineage);
		const auto decision = load_activation(connection, lineage);
		require(stored.exists && stored.phase == 2 && stored.revision == 1 &&
				stored.selected_epoch && stored.baseline_operation &&
				stored.selected_epoch->bytes == stored.epoch.bytes &&
				decision.exists && decision.state == 1 && decision.revision > 0 &&
				decision.epoch.bytes == stored.epoch.bytes &&
				decision.installation.bytes == stored.operation.bytes &&
				decision.baseline.bytes == stored.baseline_operation->bytes &&
				decision.route_count > 0 &&
				decision.verified_route_count == decision.route_count &&
				decision.unclassified_route_count == 0 &&
				digest_nonzero(decision.manifest_digest) &&
				digest_nonzero(decision.audit_digest),
			ENODATA);
		verify_baseline_receipt(connection, stored);
		(void)authenticate_opening(connection, stored);
		const auto state = one(
			connection,
			"SELECT HEX(active_epoch),revision FROM economic_lineage_state WHERE lineage=" +
				id(lineage) + " FOR UPDATE",
			2);
		auto revision = integer<uint64_t>(state[1]);
		if (state[0])
			require(parse_id(state[0]).bytes == stored.epoch.bytes, EEXIST);
		else
		{
			require(revision < std::numeric_limits<uint64_t>::max(), EOVERFLOW);
			execute(connection, "UPDATE economic_lineage_state SET active_epoch=" +
						    id(stored.epoch) +
						    ",revision=revision+1 WHERE lineage=" +
						    id(lineage) + " AND active_epoch IS NULL");
			require(mysql_affected_rows(connection) == 1, ESTALE);
			++revision;
		}
		if (new_lineage_revision)
			*new_lineage_revision = revision;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

unsigned int economic_sql_accounting_lifecycle_transaction::pause(
	MYSQL *connection, economic_sql_cutover_transaction_owner &owner,
	const critical_operation_id &lineage_id) noexcept
{
	try
	{
		require(connection && !critical_operation_id_is_zero(lineage_id), EINVAL);
		require(owner.is_valid() && owner.connection_ == connection && owner.maintenance_ &&
				owner.writer_lock_ && mysql_thread_id(connection) == owner.session_,
			EPERM);
		const auto stored = load_installation(connection, lineage_id);
		const auto activation = load_activation(connection, lineage_id);
		require(stored.exists && activation.exists && stored.phase == 2 &&
				activation.epoch.bytes == stored.epoch.bytes &&
				activation.installation.bytes == stored.operation.bytes &&
				stored.baseline_operation &&
				activation.baseline.bytes == stored.baseline_operation->bytes,
			EILSEQ);
		const auto state =
			one(connection,
			    "SELECT HEX(active_epoch) FROM economic_lineage_state WHERE lineage=" +
				    id(lineage_id) + " FOR UPDATE",
			    1);
		if (activation.state == 1)
		{
			require(state[0] && parse_id(state[0]).bytes == stored.epoch.bytes, EILSEQ);
			execute(connection, "UPDATE economic_lineage_state SET active_epoch=NULL,"
					    "revision=revision+1 WHERE lineage=" +
						    id(lineage_id) +
						    " AND active_epoch=" + id(stored.epoch));
			require(mysql_affected_rows(connection) == 1, ESTALE);
			execute(connection,
				"UPDATE economic_sql_global_activation SET state=2,revision=revision+1 "
				"WHERE lineage=" +
					id(lineage_id) + " AND state=1 AND revision=" +
					std::to_string(activation.revision));
			require(mysql_affected_rows(connection) == 1, ESTALE);
		}
		else
			require(activation.state == 2 && !state[0], EILSEQ);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}
unsigned int economic_sql_accounting_lifecycle_transaction::recover_runtime(
	MYSQL *connection, const economic_sql_lifecycle_guard &authority, bool *active) noexcept
{
	try
	{
		require(connection && active && authority.connection_ == connection &&
				!authority.maintenance_ && authority.is_valid_authority() &&
				!economic_gameplay_authority::active(),
			EPERM);
		*active = false;
		execute(connection, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
		transaction owner{ connection, mysql_thread_id(connection), true };
		execute(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT");
		const auto active_rows =
			query(connection,
			      "SELECT HEX(lineage),HEX(active_epoch) FROM economic_lineage_state "
			      "WHERE active_epoch IS NOT NULL FOR UPDATE",
			      2);
		require(active_rows.size() <= 1, EILSEQ);
		if (active_rows.empty())
		{
			execute(connection, "COMMIT");
			owner.started = false;
			return 0;
		}
		const auto lineage = parse_id(active_rows[0][0]);
		const auto epoch = parse_id(active_rows[0][1]);
		const auto stored = load_installation(connection, lineage);
		const auto activation = load_activation(connection, lineage);
		require(stored.exists && stored.phase == 2 && stored.epoch.bytes == epoch.bytes &&
				activation.exists && activation.state == 1 &&
				activation.epoch.bytes == epoch.bytes &&
				activation.installation.bytes == stored.operation.bytes &&
				stored.baseline_operation &&
				activation.baseline.bytes == stored.baseline_operation->bytes &&
				activation.route_count > 0 &&
				activation.verified_route_count == activation.route_count &&
				activation.unclassified_route_count == 0 &&
				digest_nonzero(activation.manifest_digest) &&
				digest_nonzero(activation.audit_digest),
			EILSEQ);
		verify_baseline_receipt(connection, stored);
		const auto opening = authenticate_opening(connection, stored);
		auto [snapshot, holdings] = capture_current_holdings(connection, false);
		(void)snapshot;
		require(stored.wallet_count == static_cast<uint64_t>(std::count_if(
						       holdings.begin(), holdings.end(),
						       [](const auto &value) {
							       return value.account_kind ==
								      economic_account_kind::wallet;
						       })) &&
				stored.bank_count ==
					static_cast<uint64_t>(std::count_if(
						holdings.begin(), holdings.end(),
						[](const auto &value) {
							return value.account_kind ==
							       economic_account_kind::bank;
						})),
			EILSEQ);
		economic_sql_lifecycle_request request;
		request.operation_id = stored.operation;
		request.lineage = lineage;
		request.epoch = epoch;
		const auto account_ids =
			verify_current_mappings(connection, stored, opening, holdings);
		economic_sql_lifecycle_receipt receipt;
		fill_export(request, holdings, account_ids, stored, 1, &receipt);
		require(mysql_thread_id(connection) == owner.session &&
				(connection->server_status & SERVER_STATUS_IN_TRANS),
			ENOTCONN);
		execute(connection, "COMMIT");
		owner.started = false;
		std::vector<economic_gameplay_wallet_mapping> wallets;
		std::vector<economic_gameplay_bank_mapping> banks;
		wallets.reserve(receipt.wallets.size());
		banks.reserve(receipt.banks.size());
		for (const auto &wallet : receipt.wallets)
			wallets.push_back({ wallet.pid, wallet.account });
		for (const auto &bank : receipt.banks)
			banks.push_back({ bank.name, bank.racewar, bank.account });
		const auto installed = economic_gameplay_authority::install(
			lineage, epoch, *stored.baseline_operation, wallets, banks);
		require(installed == economic_accounting_error::ok,
			installed == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
		*active = true;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

unsigned int economic_sql_accounting_lifecycle_transaction::prepare_runtime_boot(
	MYSQL *connection, const economic_sql_lifecycle_guard &authority,
	economic_sql_runtime_boot_selection *output) noexcept
{
	try
	{
		require(connection && output && !output->prepared_ && !output->finished_ &&
				!output->selected_ && !output->authority_id_ && !output->session_,
			EPERM);
		// The original guard validation includes idle/autocommit and named-lock
		// ownership. It must run before START, never inside this transaction.
		require(!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
				(connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
				authority.connection_ == connection && !authority.maintenance_ &&
				authority.is_valid_authority() &&
				!economic_gameplay_authority::active(),
			EPERM);
		execute(connection, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
		transaction owner{ connection, mysql_thread_id(connection), true };
		execute(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT");
		const auto selected = load_verified_runtime_selection(connection);
		require(mysql_thread_id(connection) == owner.session &&
				(connection->server_status & SERVER_STATUS_IN_TRANS),
			ENOTCONN);
		execute(connection, "COMMIT");
		owner.started = false;
		if (selected)
		{
			const auto installed = economic_gameplay_authority::install_sql_recovery(
				economic_gameplay_authority::sql_runtime_recovery_install_key{},
				selected->lineage, selected->epoch,
				*selected->installation.baseline_operation);
			require(installed == economic_accounting_error::ok,
				installed == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
			output->lineage_ = selected->lineage;
			output->epoch_ = selected->epoch;
			output->baseline_ = *selected->installation.baseline_operation;
			output->selected_ = true;
		}
		output->authority_id_ = authority.authority_id_;
		output->session_ = owner.session;
		output->prepared_ = true;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

unsigned int economic_sql_accounting_lifecycle_transaction::finish_runtime_boot(
	MYSQL *connection, const economic_sql_lifecycle_guard &authority,
	economic_sql_runtime_boot_selection &selected_boot, bool *active) noexcept
{
	try
	{
		require(connection && active && selected_boot.prepared_ &&
				!selected_boot.finished_ && selected_boot.authority_id_ &&
				selected_boot.authority_id_ == authority.authority_id_ &&
				selected_boot.session_ &&
				selected_boot.session_ == mysql_thread_id(connection) &&
				authority.connection_ == connection && !authority.maintenance_ &&
				!(connection->server_status & SERVER_STATUS_IN_TRANS) &&
				(connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
				authority.is_valid_authority(),
			EPERM);
		*active = false;
		require(selected_boot.selected_ ?
				economic_gameplay_authority::active_sql_recovery() :
				!economic_gameplay_authority::active(),
			EPERM);
		execute(connection, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
		transaction owner{ connection, mysql_thread_id(connection), true };
		execute(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT");
		const auto selected = load_verified_runtime_selection(connection);
		require(bool(selected) == selected_boot.selected_, ESTALE);
		if (!selected)
		{
			require(!economic_gameplay_authority::active() &&
					mysql_thread_id(connection) == owner.session &&
					(connection->server_status & SERVER_STATUS_IN_TRANS),
				ENOTCONN);
			execute(connection, "COMMIT");
			owner.started = false;
			selected_boot.finished_ = true;
			return 0;
		}
		const auto &lineage = selected->lineage;
		const auto &epoch = selected->epoch;
		const auto &stored = selected->installation;
		require(lineage.bytes == selected_boot.lineage_.bytes &&
				epoch.bytes == selected_boot.epoch_.bytes &&
				stored.baseline_operation->bytes == selected_boot.baseline_.bytes &&
				economic_gameplay_authority::sql_recovery_selection_matches(
					economic_gameplay_authority::
						sql_runtime_recovery_install_key{},
					lineage, epoch, *stored.baseline_operation),
			ESTALE);
		const auto receipt = load_verified_runtime_projection(connection, *selected);
		require(mysql_thread_id(connection) == owner.session &&
				(connection->server_status & SERVER_STATUS_IN_TRANS),
			ENOTCONN);
		execute(connection, "COMMIT");
		owner.started = false;
		std::vector<economic_gameplay_wallet_mapping> wallets;
		std::vector<economic_gameplay_bank_mapping> banks;
		wallets.reserve(receipt.wallets.size());
		banks.reserve(receipt.banks.size());
		for (const auto &wallet : receipt.wallets)
			wallets.push_back({ wallet.pid, wallet.account });
		for (const auto &bank : receipt.banks)
			banks.push_back({ bank.name, bank.racewar, bank.account });
		const auto installed = economic_gameplay_authority::finish_sql_recovery(
			economic_gameplay_authority::sql_runtime_recovery_install_key{}, lineage,
			epoch, *stored.baseline_operation, wallets, banks);
		require(installed == economic_accounting_error::ok,
			installed == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
		selected_boot.finished_ = true;
		*active = true;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}
unsigned int economic_sql_accounting_lifecycle_transaction::verify_aborted_runtime_projection(
	MYSQL *connection, economic_sql_cutover_transaction_owner &retained,
	const economic_sql_runtime_boot_selection &original) noexcept
{
	return verify_runtime_return_projection(connection, &retained, nullptr, original);
}
unsigned int economic_sql_accounting_lifecycle_transaction::verify_initialized_runtime_sources(
	MYSQL *connection, economic_sql_cutover_transaction_owner &retained,
	const economic_sql_runtime_boot_selection &original,
	const economic_sql_lifecycle_request &request,
	const economic_sql_activation_evidence &evidence,
	economic_sql_initialized_activation_verifier verify) noexcept
{
	try
	{
		require(connection && retained.connection_ == connection &&
				retained.runtime_handoff_ && original.prepared_ &&
				original.finished_ && original.selected_ &&
				original.session_ == retained.session_ &&
				original.authority_id_ == retained.sql_authority_id_ &&
				retained.is_valid(),
			EPERM);
		require(verify && evidence.route_count &&
				evidence.route_count == evidence.verified_route_count &&
				!evidence.unclassified_route_count &&
				digest_nonzero(evidence.manifest_digest) &&
				digest_nonzero(evidence.audit_digest),
			ENODATA);
		const auto selected = load_verified_runtime_selection(connection);
		require(selected && selected->lineage.bytes == original.lineage_.bytes &&
				selected->epoch.bytes == original.epoch_.bytes &&
				selected->installation.baseline_operation->bytes ==
					original.baseline_.bytes &&
				request.lineage.bytes == original.lineage_.bytes &&
				request.epoch.bytes == original.epoch_.bytes &&
				request.operation_id.bytes ==
					selected->installation.operation.bytes &&
				selected->installation.request_hash ==
					request_digest(request, selected->opening.money),
			ESTALE);
		const auto active = load_activation(connection, original.lineage_);
		require(active.exists && active.state == 1 &&
				active.epoch.bytes == request.epoch.bytes &&
				active.installation.bytes == request.operation_id.bytes &&
				active.baseline.bytes == original.baseline_.bytes &&
				active.manifest_digest == evidence.manifest_digest &&
				active.audit_digest == evidence.audit_digest &&
				active.route_count == evidence.route_count &&
				active.verified_route_count == evidence.verified_route_count &&
				!active.unclassified_route_count,
			ESTALE);
		struct context
		{
			MYSQL *connection;
			economic_sql_cutover_transaction_owner &owner;
			const economic_sql_lifecycle_request &request;
			const economic_sql_activation_evidence &evidence;
			const verified_runtime_selection &selected;
			economic_sql_initialized_activation_verifier verify;
			unsigned int error = EIO;
		} current{ connection, retained, request, evidence, *selected, verify };
		const auto inspect =
			+[](const economic_initialized_world_snapshot &world, void *raw) noexcept
		{
			auto &c = *static_cast<context *>(raw);
			try
			{
				const economic_sql_source_limits original_limits;
				require(world.version == 1 &&
						world.rows <= original_limits.maximum_rows &&
						world.cells <= original_limits.maximum_cells &&
						world.cell_bytes <=
							original_limits.maximum_cell_bytes,
					E2BIG);
				auto residual = original_limits;
				residual.maximum_rows -= world.rows;
				residual.maximum_cells -= world.cells;
				residual.maximum_cell_bytes -= world.cell_bytes;
				// Charge authentic retained world observations once BEFORE SQL reads;
				// the original single-cell allowance is unchanged throughout.
				auto captured =
					capture_complete_activation_sources(c.connection, residual);
				require(captured.totals.rows <= residual.maximum_rows &&
						captured.totals.cells <= residual.maximum_cells &&
						captured.totals.cell_bytes <=
							residual.maximum_cell_bytes,
					E2BIG);
				require(c.owner.is_valid(), EPERM);
				// Raw captures do not lock current native mappings/rows. This is the
				// original inbox -> mapping -> native/origin proof, once in this cut.
				(void)verify_current_mappings(c.connection, c.selected.installation,
							      c.selected.opening, captured.holdings,
							      &captured.native_wallets);
				// Derived metadata is not another raw provider. Bound its retained
				// allocation under the existing DTO ceiling without adding raw rows.
				require(captured.native_wallets.size() <=
							original_limits.maximum_rows &&
						captured.native_wallets.capacity() <=
							original_limits.maximum_cell_bytes /
								sizeof(economic_sql_native_mobile_wallet_lifetime),
					E2BIG);
				require(c.owner.is_valid(), EPERM);
				economic_initialized_world_money_correspondence pc_money;
				const auto compared = economic_initialized_world_compare_pc_money(
					world, captured.physical.source2, original_limits, 512,
					&pc_money);
				require(compared == economic_accounting_error::ok,
					compared == economic_accounting_error::capacity ? ENOMEM :
											  EILSEQ);
				// This is the primary PC projection comparison only. The full raw
				// sources and every original normalized finding remain independent
				// verifier inputs; complete item correspondence remains separate.
				require(std::all_of(pc_money.issue_counts.begin(),
						    pc_money.issue_counts.end(),
						    [](uint64_t count) { return count == 0; }),
					ENODATA);
				economic_initialized_world_native_money_correspondence npc_money;
				const auto native_compared =
					economic_initialized_world_compare_addressed_npc_money(
						world, captured.correspondence.native_mobile,
						captured.native_wallets, original_limits, 512,
						&npc_money);
				require(native_compared == economic_accounting_error::ok,
					native_compared == economic_accounting_error::capacity ?
						ENOMEM :
						EILSEQ);
				// Historical birth amounts are not current cash. All primary defects,
				// including unresolved first-opening NPCs, gate the independent callback.
				require(std::all_of(npc_money.issue_counts.begin(),
						    npc_money.issue_counts.end(),
						    [](uint64_t count) { return count == 0; }),
					ENODATA);
				require(c.owner.is_valid(), EPERM);
				const economic_sql_initialized_activation_view view{
					world,
					captured.physical,
					captured.room,
					captured.creation,
					captured.correspondence,
					pc_money,
					captured.native_wallets,
					npc_money,
					original_limits
				};
				execute(c.connection, "SAVEPOINT economic_sql_activation_verifier");
				const auto checked =
					c.verify(c.connection, c.request, c.evidence, view);
				execute(c.connection,
					"ROLLBACK TO SAVEPOINT economic_sql_activation_verifier");
				execute(c.connection,
					"RELEASE SAVEPOINT economic_sql_activation_verifier");
				require(c.owner.is_valid(), EPERM);
				// Callback refusal still cleans its SQL effects before returning.
				// Failed cleanup leaves the genuine transaction with the abort owner.
				require(!checked, checked);
				c.error = 0;
				return true;
			}
			catch (const failure &error)
			{
				c.error = error.code ? error.code : EIO;
			}
			catch (const std::bad_alloc &)
			{
				c.error = ENOMEM;
			}
			catch (...)
			{
				c.error = EIO;
			}
			return false;
		};
		const bool checked = economic_initialized_world_owner::with_cutover_cut(
			retained, inspect, &current);
		require(checked && retained.is_valid(), current.error ? current.error : EPERM);
		// No cached result, activation/policy installation, COMMIT, transfer or ACK.
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}
unsigned int economic_sql_accounting_lifecycle_transaction::verify_returned_runtime_projection(
	MYSQL *connection, const economic_sql_lifecycle_guard &returned,
	const economic_sql_runtime_boot_selection &original) noexcept
{
	return verify_runtime_return_projection(connection, nullptr, &returned, original);
}
unsigned int economic_sql_accounting_lifecycle_transaction::verify_runtime_return_projection(
	MYSQL *connection, economic_sql_cutover_transaction_owner *retained,
	const economic_sql_lifecycle_guard *returned,
	const economic_sql_runtime_boot_selection &original) noexcept
{
	try
	{
		// Both callers are private actual-boot-owner paths. A value/receipt or an
		// ordinary maintenance guard cannot substitute for either genuine owner.
		const auto held = [&]()
		{
			if (!connection || !original.prepared_ || !original.finished_ ||
			    !original.selected_ || bool(retained) == bool(returned))
				return false;
			if (retained)
				return retained->connection_ == connection &&
				       retained->runtime_handoff_ &&
				       !retained->runtime_commit_attempted_ &&
				       original.session_ == retained->session_ &&
				       original.authority_id_ == retained->sql_authority_id_ &&
				       retained->is_valid_for_retained_terminal(
					       economic_sql_cutover_terminal_outcome::rolled_back);
			return returned->connection_ == connection && returned->runtime_handoff_ &&
			       returned->maintenance_ && !returned->local_runtime_ &&
			       returned->local_maintenance_ && returned->writer_lock_ &&
			       returned->local_exclusive_.owns_lock() &&
			       !returned->coordinator_release_ &&
			       !returned->coordinator_generation_ &&
			       !returned->coordinator_lease_id_ &&
			       original.session_ == returned->session_ &&
			       original.authority_id_ == returned->authority_id_ &&
			       critical_command_coordinator_lifecycle_guard_held_by_current_thread() &&
			       returned->is_valid_authority();
		};
		require(held(), EPERM);
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup read(connection, cleanup);
		read.starting();
		execute(connection, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");
		execute(connection, "START TRANSACTION WITH CONSISTENT SNAPSHOT");
		const auto selected = load_verified_runtime_selection(connection);
		require(selected && selected->lineage.bytes == original.lineage_.bytes &&
				selected->epoch.bytes == original.epoch_.bytes &&
				selected->installation.baseline_operation->bytes ==
					original.baseline_.bytes,
			ESTALE);
		const auto receipt = load_verified_runtime_projection(connection, *selected);
		std::vector<economic_gameplay_wallet_mapping> wallets;
		std::vector<economic_gameplay_bank_mapping> banks;
		wallets.reserve(receipt.wallets.size());
		banks.reserve(receipt.banks.size());
		for (const auto &wallet : receipt.wallets)
			wallets.push_back({ wallet.pid, wallet.account });
		for (const auto &bank : receipt.banks)
			banks.push_back({ bank.name, bank.racewar, bank.account });
		require(mysql_thread_id(connection) == original.session_ &&
				(connection->server_status & SERVER_STATUS_IN_TRANS),
			ENOTCONN);
		read.finish();
		require(read.same_session() && cleanup.rollback_confirmed &&
				cleanup.disposition ==
					player_sql_cleanup_disposition::idle_verified &&
				!cleanup.cleanup_error && held(),
			ENOTCONN);
		require(economic_gameplay_authority::sql_runtime_projection_matches(
				economic_gameplay_authority::sql_runtime_recovery_install_key{},
				selected->lineage, selected->epoch,
				*selected->installation.baseline_operation, wallets, banks),
			ESTALE);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code ? error.code : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

#endif
