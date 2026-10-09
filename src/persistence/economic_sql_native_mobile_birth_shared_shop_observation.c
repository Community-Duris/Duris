#include "persistence/economic_sql_native_mobile_birth_shared_shop_observation.h"

#include <cerrno>
#ifndef __NO_MYSQL__
#include "item/item_transfer_repository.h"
#include "economy/shop_trade_command.h"
#include "player/player_snapshot_codec.h"
#include "persistence/economic_sql_source_snapshot.h"
#include "core/structs.h"
#include <algorithm>
#include <charconv>
#include <climits>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <type_traits>
#include <utility>

namespace
{
using cells = std::vector<std::optional<std::string>>;
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code ? code : EIO };
}
void active(MYSQL *connection, unsigned long session)
{
	require(connection && session && mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS),
		ENOTCONN);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect,
		EPERM);
}
using result = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
result read(MYSQL *connection, const std::string &query, size_t fields, size_t maximum)
{
	if (mysql_real_query(connection, query.data(), query.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
	result rows(mysql_store_result(connection), mysql_free_result);
	require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_fields(rows.get()) == fields);
	require(mysql_num_rows(rows.get()) <= maximum, E2BIG);
	return rows;
}
cells row_values(MYSQL_RES *rows, MYSQL_ROW row, size_t count)
{
	const auto *lengths = mysql_fetch_lengths(rows);
	require(row && lengths);
	cells value;
	value.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		require(lengths[i] <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES, E2BIG);
		value.push_back(
			row[i] ? std::optional<std::string>(std::string(row[i], lengths[i])) :
				 std::nullopt);
	}
	return value;
}
template <class T> T number(const std::optional<std::string> &value)
{
	require(value && !value->empty());
	T result{};
	const auto parsed = std::from_chars(value->data(), value->data() + value->size(), result);
	require(parsed.ec == std::errc{} && parsed.ptr == value->data() + value->size());
	return result;
}
struct decoded
{
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_frozen_intent intent;
	std::vector<uint8_t> bytes;
};
decoded decode(const critical_command &command)
{
	decoded value;
	const auto code = native_mobile_birth_cash_role_command_decode(command, &value.image,
								       &value.recipes, &value.role);
	require(code == economic_accounting_error::ok,
		code == economic_accounting_error::capacity ? ENOMEM : EINVAL);
	require(value.role.role == native_mobile_birth_cash_role::shared_shopkeeper, ENOTSUP);
	require(economic_intent_decode(command.accounting_intent, &value.intent) ==
			economic_accounting_error::ok,
		EINVAL);
	require(value.image.items.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS, E2BIG);
	require(critical_command_encode(command, &value.bytes) == critical_command_codec_result::ok,
		EINVAL);
	return value;
}
std::string owner_predicate(const item_owner_identity &owner)
{
	return "owner_type=" + std::to_string(static_cast<uint8_t>(owner.type)) +
	       " AND owner_id=" + std::to_string(owner.id);
}
economic_sql_native_mobile_birth_shared_shop_before before_values(MYSQL *connection,
								  const decoded &identity)
{
	economic_sql_native_mobile_birth_shared_shop_before value;
	value.original_session = mysql_thread_id(connection);
	active(connection, value.original_session);
	const auto &metadata = identity.intent.admission.metadata;
	const auto error = economic_sql_lock_authority(connection, metadata.lineage, metadata.epoch,
						       {}, &value.authority);
	require(!error, error);
	value.original_command = identity.bytes;
	value.shop_id = static_cast<uint32_t>(identity.role.original.reset_shop_index);
	value.shop_owner = { item_owner_type::shopkeeper, item_shopkeeper_owner_id(value.shop_id),
			     0 };
	require(item_owner_identity_valid(value.shop_owner), EINVAL);
	auto rows = read(
		connection,
		"SELECT id,shop_id,mob_vnum,room_vnum,save_time,cash,shop_revision,keeper_roaming,updated_at,runtime_payload_checkpoint_revision FROM shopkeepers WHERE shop_id=" +
			std::to_string(value.shop_id) + " FOR UPDATE",
		10, 1);
	if (mysql_num_rows(rows.get()))
	{
		value.keeper_cells = row_values(rows.get(), mysql_fetch_row(rows.get()), 10);
		const auto &r = value.keeper_cells;
		const auto id = number<uint64_t>(r[0]);
		require(id && id <= UINT32_MAX && number<uint32_t>(r[1]) == value.shop_id, ESTALE);
		value.keeper_present = true;
		value.keeper_id = static_cast<uint32_t>(id);
		value.keeper_vnum = number<int32_t>(r[2]);
		require(value.keeper_vnum == identity.role.original.mobile_vnum, ESTALE);
		require(r[5] && r[7], ENODATA);
		value.keeper_cash = number<int64_t>(r[5]);
		value.shop_revision = number<uint64_t>(r[6]);
		const auto roaming = number<uint8_t>(r[7]);
		require(value.keeper_cash >= 0 && value.keeper_cash <= INT_MAX && roaming <= 1,
			ESTALE);
		value.keeper_roaming = roaming != 0;
		value.payload_checkpoint_present = r[9].has_value();
		if (value.payload_checkpoint_present)
		{
			value.payload_checkpoint_revision = number<uint64_t>(r[9]);
			require(value.payload_checkpoint_revision &&
					value.payload_checkpoint_revision <= value.shop_revision,
				ESTALE);
		}
	}
	// There is exactly one participating owner: original SHOP slot+1, context0.
	// Missing remains absent; the existing lock API cannot fetch a missing row.
	auto counters = read(connection,
			     "SELECT revision FROM item_owner_revision WHERE " +
				     owner_predicate(value.shop_owner) +
				     " AND owner_context_id=0 FOR UPDATE",
			     1, 1);
	if (mysql_num_rows(counters.get()))
	{
		value.owner_present = true;
		value.owner_revision = number<uint64_t>(
			row_values(counters.get(), mysql_fetch_row(counters.get()), 1)[0]);
		uint64_t confirmed = 0;
		errno = 0;
		const bool locked = item_transfer_repository_lock_owner(
			connection, value.shop_owner, &confirmed);
		require(locked, errno ? static_cast<unsigned int>(errno) : EIO);
		require(confirmed == value.owner_revision, ESTALE);
	}
	active(connection, value.original_session);
	return value;
}
bool same_before(const economic_sql_native_mobile_birth_shared_shop_before &a,
		 const economic_sql_native_mobile_birth_shared_shop_before &b)
{
	return a.original_session == b.original_session &&
	       a.original_command == b.original_command &&
	       a.authority.lineage.bytes == b.authority.lineage.bytes &&
	       a.authority.epoch.bytes == b.authority.epoch.bytes &&
	       a.authority.lineage_revision == b.authority.lineage_revision &&
	       a.authority.mappings.empty() && b.authority.mappings.empty() &&
	       a.shop_id == b.shop_id && a.keeper_id == b.keeper_id &&
	       item_owner_identity_equal(a.shop_owner, b.shop_owner) &&
	       a.keeper_present == b.keeper_present && a.owner_present == b.owner_present &&
	       a.shop_revision == b.shop_revision && a.owner_revision == b.owner_revision &&
	       a.keeper_cash == b.keeper_cash && a.keeper_vnum == b.keeper_vnum &&
	       a.keeper_roaming == b.keeper_roaming &&
	       a.payload_checkpoint_present == b.payload_checkpoint_present &&
	       a.payload_checkpoint_revision == b.payload_checkpoint_revision &&
	       a.keeper_cells == b.keeper_cells;
}
std::map<uint64_t, uint64_t> routes(MYSQL *connection, uint32_t keeper, bool locked)
{
	if (!keeper)
		return {};
	auto rows = read(connection,
			 "SELECT id,obj_uid FROM shopkeeper_items WHERE shopkeeper_id=" +
				 std::to_string(keeper) + " ORDER BY id LIMIT " +
				 std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) +
				 (locked ? " FOR UPDATE" : ""),
			 2, PLAYER_SNAPSHOT_MAX_OBJECTS);
	std::map<uint64_t, uint64_t> values;
	std::set<uint64_t> uids;
	while (auto row = mysql_fetch_row(rows.get()))
	{
		const auto r = row_values(rows.get(), row, 2);
		require(bool(r[1]), ENODATA);
		const auto id = number<uint64_t>(r[0]), uid = number<uint64_t>(r[1]);
		require(id && uid && uid != UINT64_MAX && uids.insert(uid).second &&
				values.emplace(id, uid).second,
			ESTALE);
	}
	require(values.size() == mysql_num_rows(rows.get()), EIO);
	return values;
}
std::string uid_list(const std::set<uint64_t> &values)
{
	std::string out;
	for (uint64_t uid : values)
	{
		if (!out.empty())
			out += ',';
		out += std::to_string(uid);
	}
	return out;
}
std::vector<economic_sql_native_mobile_birth_shared_shop_custody>
custody(MYSQL *connection, const decoded &identity,
	const economic_sql_native_mobile_birth_shared_shop_before &before,
	const std::map<uint64_t, uint64_t> &physical, bool initial = true)
{
	std::set<uint64_t> claimed, born;
	for (const auto &item : identity.image.items)
	{
		claimed.insert(item.object_uid);
		born.insert(item.object_uid);
	}
	for (const auto &[id, uid] : physical)
	{
		(void)id;
		claimed.insert(uid);
	}
	// Nonlocking routing only under the already held SHOP owner/keeper locks.
	// Include all contexts/history so malformed or unphysical rows cannot vanish.
	auto seeds = read(connection,
			  "SELECT item_uid FROM item_current_owner WHERE " +
				  owner_predicate(before.shop_owner) + " ORDER BY item_uid LIMIT " +
				  std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1),
			  1, PLAYER_SNAPSHOT_MAX_OBJECTS);
	while (auto row = mysql_fetch_row(seeds.get()))
	{
		const auto uid = number<uint64_t>(row_values(seeds.get(), row, 1)[0]);
		require(uid && uid != UINT64_MAX, ESTALE);
		claimed.insert(uid);
	}
	// Same complete original SQL SHOP publication bound, not a new budget.
	constexpr size_t limit = 2 * PLAYER_SNAPSHOT_MAX_OBJECTS + SHOP_TRADE_MAX_ITEMS +
				 PLAYER_SNAPSHOT_MAX_DEPTH + 1;
	require(claimed.size() <= limit, E2BIG);
	std::string scope = "(" + owner_predicate(before.shop_owner) + ")";
	if (!claimed.empty())
	{
		const auto list = uid_list(claimed);
		scope += " OR item_uid IN(" + list + ") OR ((root_item_uid IN(" + list +
			 ") OR parent_item_uid IN(" + list + ")) AND state=" +
			 std::to_string(static_cast<uint8_t>(item_custody_state::active)) + ")";
	}
	auto rows = read(
		connection,
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot,OCTET_LENGTH(coin_payload) FROM item_current_owner WHERE " +
			scope + " ORDER BY item_uid LIMIT " + std::to_string(limit + 1) +
			" FOR UPDATE",
		11, limit);
	std::vector<economic_sql_native_mobile_birth_shared_shop_custody> out;
	std::vector<uint64_t> payload_sizes;
	uint64_t cell_bytes = 0;
	const auto maximum_bytes = economic_sql_source_limits{}.maximum_cell_bytes;
	const auto charge = [&](uint64_t count)
	{
		require(count <= maximum_bytes - cell_bytes, E2BIG);
		cell_bytes += count;
	};
	uint64_t previous = 0;
	while (auto row = mysql_fetch_row(rows.get()))
	{
		const auto r = row_values(rows.get(), row, 11);
		for (const auto &cell : r)
			if (cell)
				charge(cell->size());
		economic_sql_native_mobile_birth_shared_shop_custody value;
		value.snapshot.uid = number<uint64_t>(r[0]);
		const auto type = number<uint8_t>(r[3]), state = number<uint8_t>(r[8]);
		const auto slot = number<uint16_t>(r[9]);
		require(value.snapshot.uid > previous && value.snapshot.uid != UINT64_MAX &&
				type <= static_cast<uint8_t>(item_owner_type::native_mobile) &&
				state <= static_cast<uint8_t>(item_custody_state::quarantined) &&
				slot <= MAX_WEAR,
			ESTALE);
		previous = value.snapshot.uid;
		value.snapshot.position = { { static_cast<item_owner_type>(type),
					      number<uint64_t>(r[4]), number<uint64_t>(r[5]) },
					    number<uint64_t>(r[1]),
					    number<uint64_t>(r[2]),
					    number<uint64_t>(r[6]),
					    static_cast<item_custody_state>(state),
					    slot };
		value.vnum = number<int32_t>(r[7]);
		require(item_owner_identity_valid(value.snapshot.position.owner) &&
				value.snapshot.position.revision,
			ESTALE);
		const auto &position = value.snapshot.position;
		if (initial)
			require(!born.count(value.snapshot.uid) &&
					!(position.state == item_custody_state::active &&
					  (born.count(position.root_uid) ||
					   born.count(position.parent_uid))),
				EEXIST);
		uint64_t payload_size = 0;
		value.coin_payload_present = r[10].has_value();
		if (value.coin_payload_present)
		{
			value.coin_payload_bytes = number<uint64_t>(r[10]);
			if (position.owner.type == before.shop_owner.type &&
			    position.owner.id == before.shop_owner.id)
			{
				payload_size = value.coin_payload_bytes;
				require(payload_size && payload_size <= PLAYER_SNAPSHOT_MAX_BYTES,
					E2BIG);
				charge(payload_size);
				value.coin_payload.emplace();
			}
		}
		if (value.snapshot.position.owner.type == before.shop_owner.type &&
		    value.snapshot.position.owner.id == before.shop_owner.id)
			require(claimed.count(value.snapshot.uid), ESTALE);
		payload_sizes.push_back(payload_size);
		out.push_back(std::move(value));
	}
	require(out.size() == mysql_num_rows(rows.get()), EIO);
	// First acquire the full globally sorted cut and preflight this reader's
	// retained-cell bound, using the original source maximum_cell_bytes value.
	// This is not producer/hold memory permission. Fetch selected SHOP history
	// bytes only after every custody lock; foreign claims retain metadata only;
	// mysql_store_result never buffers thousands of maximum-size history blobs.
	for (size_t index = 0; index < out.size(); ++index)
	{
		if (!out[index].coin_payload)
			continue;
		auto payload = read(connection,
				    "SELECT OCTET_LENGTH(coin_payload),SUBSTRING(coin_payload,1," +
					    std::to_string(PLAYER_SNAPSHOT_MAX_BYTES + 1) +
					    ") FROM item_current_owner WHERE item_uid=" +
					    std::to_string(out[index].snapshot.uid) + " FOR UPDATE",
				    2, 1);
		require(mysql_num_rows(payload.get()) == 1, ESTALE);
		const auto r = row_values(payload.get(), mysql_fetch_row(payload.get()), 2);
		require(r[1] && number<uint64_t>(r[0]) == payload_sizes[index] &&
				r[1]->size() == payload_sizes[index],
			ESTALE);
		out[index].coin_payload->assign(r[1]->begin(), r[1]->end());
	}
	return out;
}
void physical_absence(MYSQL *connection, uint64_t uid, const char *allowed = nullptr)
{
	for (const char *table :
	     { "player_items", "shopkeeper_items", "player_pet_items", "locker_items",
	       "account_locker_items", "corpse_items", "saved_items", "siege_items" })
	{
		auto rows = read(connection,
				 "SELECT id FROM " + std::string(table) + " WHERE obj_uid=" +
					 std::to_string(uid) + " LIMIT 2 FOR UPDATE",
				 1, 2);
		require(mysql_num_rows(rows.get()) ==
				(allowed && std::string(table) == allowed ? 1 : 0),
			ESTALE);
	}
}
void prove_stock(MYSQL *connection, const decoded &identity,
		 economic_sql_native_mobile_birth_shared_shop_stock &out, bool initial = true)
{
	std::set<uint64_t> born;
	for (const auto &item : identity.image.items)
		born.insert(item.object_uid);
	std::map<uint64_t, const economic_sql_native_mobile_birth_shared_shop_custody *> by_uid;
	for (const auto &row : out.custody)
	{
		const auto &p = row.snapshot.position;
		require(by_uid.emplace(row.snapshot.uid, &row).second, ESTALE);
		if (initial)
			require(!born.count(row.snapshot.uid) &&
					!(p.state == item_custody_state::active &&
					  (born.count(p.root_uid) || born.count(p.parent_uid))),
				EEXIST);
		if (p.state == item_custody_state::active &&
		    item_owner_identity_equal(p.owner, out.before.shop_owner))
			require(out.keeper_items.count(row.snapshot.uid), ENODATA);
		else if (p.state == item_custody_state::active &&
			 (out.keeper_items.count(p.root_uid) ||
			  out.keeper_items.count(p.parent_uid)))
			require(false, ESTALE);
	}
	require(out.keeper_items.size() == out.physical_routes.size(), ENODATA);
	for (const auto &[id, uid] : out.physical_routes)
	{
		const auto native = out.keeper_items.find(uid);
		const auto raw = by_uid.find(uid);
		require(native != out.keeper_items.end() && raw != by_uid.end(), ENODATA);
		const auto &n = native->second;
		const auto &r = *raw->second;
		const auto &pos = r.snapshot.position;
		require(n.id == id && n.payload_present &&
				item_owner_identity_equal(pos.owner, out.before.shop_owner) &&
				pos.state == item_custody_state::active &&
				pos.root_uid == n.root_uid && pos.revision == n.revision &&
				pos.equipment_slot == n.slot && r.vnum == n.item.vnum,
			ESTALE);
		uint64_t parent = 0;
		if (n.parent_id)
		{
			const auto found = out.physical_routes.find(n.parent_id);
			require(found != out.physical_routes.end(), ESTALE);
			parent = found->second;
		}
		require(parent == pos.parent_uid, ESTALE);
		if (r.coin_payload)
		{
			require(n.item.type == ITEM_MONEY &&
					std::none_of(n.item.values.begin(),
						     n.item.values.begin() + 4,
						     [](int32_t value) { return value < 0; }),
				EILSEQ);
			std::vector<player_item_snapshot> decoded;
			const auto decode_code = player_item_snapshot_list_decode(
				r.coin_payload->data(), r.coin_payload->size(), &decoded);
			require(decode_code == player_snapshot_codec_result::ok,
				decode_code == player_snapshot_codec_result::allocation_failure ?
					ENOMEM :
					EILSEQ);
			require(decoded.size() == 1 && decoded.front().type == ITEM_MONEY, EILSEQ);
			std::vector<uint8_t> canonical;
			const auto encode_code =
				player_item_snapshot_list_encode(decoded, &canonical);
			require(encode_code == player_snapshot_codec_result::ok,
				encode_code == player_snapshot_codec_result::allocation_failure ?
					ENOMEM :
					EILSEQ);
			require(canonical == *r.coin_payload, EILSEQ);
			// The original independent coin body has no topology authority.
			// Actual custody/native position is proved above; compare every other
			// property using the established native-player cut's neutral positions.
			auto literal = n.item;
			literal.parent_index = decoded.front().parent_index =
				PLAYER_SNAPSHOT_NO_PARENT;
			literal.equipment_slot = decoded.front().equipment_slot = 0;
			std::vector<uint8_t> native_bytes, retained_bytes;
			const auto native_code =
				player_item_snapshot_list_encode({ literal }, &native_bytes);
			const auto retained_code =
				player_item_snapshot_list_encode(decoded, &retained_bytes);
			require(native_code == player_snapshot_codec_result::ok &&
					retained_code == player_snapshot_codec_result::ok,
				native_code == player_snapshot_codec_result::allocation_failure ||
						retained_code == player_snapshot_codec_result::
									 allocation_failure ?
					ENOMEM :
					EILSEQ);
			require(native_bytes == retained_bytes, ESTALE);
		}
		physical_absence(connection, uid, "shopkeeper_items");
	}
	if (initial)
	{
		for (uint64_t uid : born)
			physical_absence(connection, uid);
	}
	else
	{
		require(out.before.keeper_present && out.keeper_items.size() == born.size(),
			ESTALE);
		for (uint64_t uid : born)
			require(out.keeper_items.count(uid), ESTALE);
	}
}
void affects(MYSQL *connection, economic_sql_native_mobile_birth_shared_shop_stock &out)
{
	if (!out.before.keeper_present)
		return;
	auto rows = read(
		connection,
		"SELECT id,shopkeeper_id,type,duration,modifier,location,bitvector1,bitvector2,bitvector3,bitvector4,bitvector5 FROM shopkeeper_affects WHERE shopkeeper_id=" +
			std::to_string(out.before.keeper_id) + " ORDER BY id LIMIT " +
			std::to_string(PLAYER_SNAPSHOT_MAX_ROWS + 1) + " FOR UPDATE",
		11, PLAYER_SNAPSHOT_MAX_ROWS);
	uint64_t previous = 0;
	while (auto row = mysql_fetch_row(rows.get()))
	{
		auto r = row_values(rows.get(), row, 11);
		const auto id = number<uint64_t>(r[0]);
		require(id > previous && number<uint32_t>(r[1]) == out.before.keeper_id, ESTALE);
		previous = id;
		for (size_t i = 2; i < 6; ++i)
			(void)number<int32_t>(r[i]);
		for (size_t i = 6; i < 11; ++i)
			(void)number<uint64_t>(r[i]);
		out.keeper_affects.push_back(std::move(r));
	}
	require(out.keeper_affects.size() == mysql_num_rows(rows.get()), EIO);
}
} // namespace
#endif

unsigned int economic_sql_native_mobile_birth_shared_shop_observe_before_locked(
	MYSQL *connection, const critical_command &command,
	economic_sql_native_mobile_birth_shared_shop_before *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		require(connection && output, EINVAL);
		const auto identity = decode(command);
		auto value = before_values(connection, identity);
		active(connection, value.original_session);
		static_assert(std::is_nothrow_move_assignable_v<
			      economic_sql_native_mobile_birth_shared_shop_before>);
		*output = std::move(value);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EINVAL;
	}
#endif
}
unsigned int economic_sql_native_mobile_birth_shared_shop_observe_stock_locked(
	MYSQL *connection, const critical_command &command,
	const economic_sql_native_mobile_birth_shared_shop_before &before,
	economic_sql_native_mobile_birth_shared_shop_stock *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)before;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		require(connection && output, EINVAL);
		active(connection, before.original_session);
		const auto identity = decode(command);
		economic_sql_native_mobile_birth_shared_shop_stock value;
		value.before = before_values(connection, identity);
		require(same_before(value.before, before), ESTALE);
		value.physical_routes = routes(connection, value.before.keeper_id, false);
		value.custody = custody(connection, identity, value.before, value.physical_routes);
		require(routes(connection, value.before.keeper_id, true) == value.physical_routes,
			ESTALE);
		if (value.before.keeper_present)
		{
			errno = 0;
			const bool complete = shop_item_runtime_keeper_image(
				connection, value.before.keeper_id, value.before.shop_id,
				value.before.keeper_vnum, &value.keeper_items);
			require(complete, errno ? static_cast<unsigned int>(errno) : EIO);
		}
		prove_stock(connection, identity, value);
		affects(connection, value);
		const auto final = before_values(connection, identity);
		require(same_before(value.before, final), ESTALE);
		active(connection, before.original_session);
		static_assert(std::is_nothrow_move_assignable_v<
			      economic_sql_native_mobile_birth_shared_shop_stock>);
		*output = std::move(value);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EINVAL;
	}
#endif
}

unsigned int economic_sql_native_mobile_birth_shared_shop_observe_current_stock_locked(
	MYSQL *connection, const critical_command &command,
	const economic_sql_native_mobile_birth_shared_shop_before &before,
	economic_sql_native_mobile_birth_shared_shop_stock *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)before;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		require(connection && output, EINVAL);
		active(connection, before.original_session);
		const auto identity = decode(command);
		economic_sql_native_mobile_birth_shared_shop_stock value;
		value.before = before_values(connection, identity);
		require(same_before(value.before, before) && value.before.keeper_present, ESTALE);
		value.physical_routes = routes(connection, value.before.keeper_id, false);
		value.custody =
			custody(connection, identity, value.before, value.physical_routes, false);
		require(routes(connection, value.before.keeper_id, true) == value.physical_routes,
			ESTALE);
		if (value.before.keeper_present)
		{
			errno = 0;
			const bool complete = shop_item_runtime_keeper_image(
				connection, value.before.keeper_id, value.before.shop_id,
				value.before.keeper_vnum, &value.keeper_items);
			require(complete, errno ? static_cast<unsigned int>(errno) : EIO);
		}
		prove_stock(connection, identity, value, false);
		affects(connection, value);
		const auto final = before_values(connection, identity);
		require(same_before(value.before, final), ESTALE);
		active(connection, before.original_session);
		static_assert(std::is_nothrow_move_assignable_v<
			      economic_sql_native_mobile_birth_shared_shop_stock>);
		*output = std::move(value);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EINVAL;
	}
#endif
}
