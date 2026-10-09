#include "world/quest_mobile_published_world_owner.h"
#include "world/quest_mobile_native_birth.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include "item/item_transfer_repository.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#ifndef __NO_MYSQL__
#include "persistence/economic_sql_source_snapshot.h"
#include <openssl/sha.h>

namespace
{
struct failure
{
	unsigned int code;
};
void require(bool value, unsigned int code = EILSEQ)
{
	if (!value)
		throw failure{ code ? code : EIO };
}
void active(MYSQL *connection, unsigned long session)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	require(connection && session && mysql_thread_id(connection) == session &&
			(connection->server_status & SERVER_STATUS_IN_TRANS) &&
			(connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
			!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
			!reconnect,
		ENOTCONN);
}
template <class T> T integer(const std::optional<std::string> &cell)
{
	require(cell && !cell->empty() && cell->size() <= 20);
	T value{};
	const auto parsed = std::from_chars(cell->data(), cell->data() + cell->size(), value);
	require(parsed.ec == std::errc{} && parsed.ptr == cell->data() + cell->size() &&
		std::to_string(value) == *cell);
	return value;
}
using cells = std::vector<std::optional<std::string>>;
using digest = std::array<uint8_t, SHA256_DIGEST_LENGTH>;
digest published_origin_digest(std::span<const uint8_t> bytes)
{
	digest result{};
	require(SHA256(bytes.data(), bytes.size(), result.data()) != nullptr, EIO);
	return result;
}
std::string operation(const critical_operation_id &id)
{
	char value[2 * CRITICAL_COMMAND_ID_BYTES + 1]{};
	require(critical_operation_id_to_hex(id, value, sizeof(value)));
	return std::string("X'") + value + "'";
}
bool exact_operation(const std::optional<std::string> &cell, const critical_operation_id &id)
{
	return cell && cell->size() == id.bytes.size() &&
	       std::equal(id.bytes.begin(), id.bytes.end(),
			  reinterpret_cast<const uint8_t *>(cell->data()));
}
// Streaming rows keep existing source row/cell/byte ceilings. The original
// bounded native/origin codecs retain their own4MiB/32MiB single body limits.
std::vector<cells> query(MYSQL *connection, unsigned long session, const std::string &sql,
			 size_t maximum_rows, std::span<const size_t> column_bounds)
{
	active(connection, session);
	require(!mysql_real_query(connection, sql.data(), sql.size()), mysql_errno(connection));
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_use_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection));
	require(mysql_num_fields(result.get()) == column_bounds.size());
	const economic_sql_source_limits limits{};
	std::vector<cells> values;
	uint64_t byte_count = 0, cell_count = 0;
	while (MYSQL_ROW row = mysql_fetch_row(result.get()))
	{
		require(values.size() < maximum_rows && maximum_rows <= limits.maximum_rows, E2BIG);
		const auto lengths = mysql_fetch_lengths(result.get());
		require(lengths != nullptr);
		cells value;
		value.reserve(column_bounds.size());
		for (size_t column = 0; column < column_bounds.size(); ++column)
		{
			require(lengths[column] <= column_bounds[column] &&
					lengths[column] <= limits.maximum_cell_bytes - byte_count &&
					cell_count < limits.maximum_cells,
				E2BIG);
			byte_count += lengths[column];
			++cell_count;
			value.push_back(row[column] ? std::optional<std::string>(std::string(
							      row[column], lengths[column])) :
						      std::nullopt);
		}
		values.push_back(std::move(value));
	}
	require(!mysql_errno(connection), mysql_errno(connection));
	result.reset();
	active(connection, session);
	return values;
}
struct observed_native
{
	uint64_t id = 0;
	std::vector<uint8_t> canonical;
	quest_mobile_native_reference reference;
	native_mobile_wallet_origin wallet;
	digest origin_hash{};
	uint64_t publication_revision = 0;
	// Selected only from the authenticated complete original terminal attachment.
	bool ordinary_cash_role = false;
	cells mapping;
};
std::vector<observed_native> observe_catalog(MYSQL *connection, unsigned long session)
{
	const economic_sql_source_limits limits{};
	const std::array<size_t, 6> bounds{ 20, 20, 20, 3, 20, PLAYER_SNAPSHOT_MAX_BYTES + 1 };
	const auto rows =
		query(connection, session,
		      "SELECT mobile_instance_id,mobile_revision,stock_revision,lifetime_state,"
		      "OCTET_LENGTH(canonical_image),SUBSTRING(canonical_image,1," +
			      std::to_string(PLAYER_SNAPSHOT_MAX_BYTES + 1) +
			      ") FROM quest_mobile_native "
			      "ORDER BY mobile_instance_id LIMIT " +
			      std::to_string(limits.maximum_rows + 1),
		      limits.maximum_rows, bounds);
	std::vector<observed_native> values;
	values.reserve(rows.size());
	uint64_t previous = 0;
	for (const auto &row : rows)
	{
		const auto id = integer<uint64_t>(row[0]);
		const auto size = integer<uint64_t>(row[4]);
		require(id && id != UINT64_MAX && id > previous && row[5] && size &&
			size <= PLAYER_SNAPSHOT_MAX_BYTES && row[5]->size() == size);
		previous = id;
		observed_native value;
		value.id = id;
		value.canonical.assign(reinterpret_cast<const uint8_t *>(row[5]->data()),
				       reinterpret_cast<const uint8_t *>(row[5]->data()) +
					       row[5]->size());
		quest_mobile_native_image image;
		std::vector<uint8_t> encoded;
		require(quest_mobile_native_image_decode(value.canonical, &image) ==
				player_snapshot_codec_result::ok &&
			quest_mobile_native_image_encode(image, &encoded) ==
				player_snapshot_codec_result::ok &&
			encoded == value.canonical && image.reference.mobile_instance_id == id &&
			image.reference.mobile_revision == integer<uint64_t>(row[1]) &&
			image.reference.stock_revision == integer<uint64_t>(row[2]) &&
			static_cast<uint8_t>(image.state) == integer<uint8_t>(row[3]) &&
			image.state == quest_mobile_lifetime_state::live && image.cash.has_value());
		// No authenticated retirement owner exists in this supported domain.
		value.reference = image.reference;
		values.push_back(std::move(value));
	}
	return values;
}
critical_native_recovery_envelope observe_origin(MYSQL *connection, unsigned long session,
						 const quest_mobile_native_reference &reference)
{
	const std::array<size_t, 4> bounds{ CRITICAL_COMMAND_ID_BYTES, 20, 20,
					    CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES + 1 };
	const auto rows =
		query(connection, session,
		      "SELECT birth_operation,publication_revision,OCTET_LENGTH(canonical_origin),"
		      "SUBSTRING(canonical_origin,1," +
			      std::to_string(CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES + 1) +
			      ") FROM quest_mobile_native_birth_origin WHERE mobile_instance_id=" +
			      std::to_string(reference.mobile_instance_id) + " LIMIT 2",
		      2, bounds);
	require(rows.size() == 1 && exact_operation(rows[0][0], reference.birth_operation));
	const auto size = integer<uint64_t>(rows[0][2]);
	require(size && size <= CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES && rows[0][3] &&
		rows[0][3]->size() == size);
	critical_native_recovery_envelope result;
	result.phase = critical_native_recovery_phase::continuation_pending;
	result.revision = integer<uint64_t>(rows[0][1]);
	result.attachment.assign(reinterpret_cast<const uint8_t *>(rows[0][3]->data()),
				 reinterpret_cast<const uint8_t *>(rows[0][3]->data()) + size);
	// Both codecs read the actual bounded original bytes with strong outputs.
	// Ordinary success requires full original NMB4/MBR4; historical success
	// requires the unchanged original constructor policy. No SQL failure or
	// CURRENT image is used to choose or reconstruct an original command.
	const bool ordinary = native_mobile_birth_cash_role_recovery_original_command_decode(
				      result.attachment, &result.command) ==
			      economic_accounting_error::ok;
	require((ordinary ||
		 native_mobile_birth_recovery_original_command_decode(
			 result.attachment, &result.command) == economic_accounting_error::ok) &&
		critical_operation_id_equal(result.command.operation_id,
					    reference.birth_operation) &&
		(ordinary ? native_mobile_birth_cash_role_recovery_terminal(result) :
			    native_mobile_birth_recovery_terminal(result)));
	return result;
}
cells observe_mapping(MYSQL *connection, unsigned long session,
		      const native_mobile_wallet_origin &wallet)
{
	const std::array<size_t, 11> bounds{ 20,
					     CRITICAL_COMMAND_ID_BYTES,
					     5,
					     20,
					     5,
					     5,
					     20,
					     20,
					     CRITICAL_COMMAND_ID_BYTES,
					     CRITICAL_COMMAND_ID_BYTES,
					     20 };
	const auto rows = query(
		connection, session,
		"SELECT mapping_id,lineage,account_kind,context_id,backend_kind,locator_kind,native_id,"
		"active_native_id,creating_operation_id,retiring_operation_id,revision FROM economic_account_mapping WHERE lineage=" +
			operation(wallet.lineage) +
			" AND mapping_id=" + std::to_string(wallet.wallet_mapping_id) + " LIMIT 2",
		2, bounds);
	require(rows.size() == 1);
	const auto &row = rows[0];
	require(integer<uint64_t>(row[0]) == wallet.wallet_mapping_id &&
		exact_operation(row[1], wallet.lineage) &&
		integer<uint16_t>(row[2]) == static_cast<uint16_t>(economic_account_kind::wallet) &&
		integer<uint64_t>(row[3]) == ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT &&
		integer<uint16_t>(row[4]) == 1 &&
		integer<uint16_t>(row[5]) == ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR &&
		integer<uint64_t>(row[6]) == wallet.mobile_instance_id &&
		integer<uint64_t>(row[7]) == wallet.mobile_instance_id &&
		exact_operation(row[8], wallet.birth_operation) && !row[9] &&
		integer<uint64_t>(row[10]) == 0);
	return row;
}
// Outside custody is observed only: this bounded ID/state/context cut catches
// orphan native claims and malformed contexts without acquiring outside locks.
std::vector<cells> observe_active_native_claims(MYSQL *connection, unsigned long session,
						const std::vector<observed_native> &catalog)
{
	const economic_sql_source_limits limits{};
	const std::array<size_t, 4> bounds{ 20, 20, 20, 3 };
	auto rows = query(
		connection, session,
		"SELECT item_uid,owner_id,owner_context_id,state FROM item_current_owner WHERE owner_type=" +
			std::to_string(static_cast<uint8_t>(item_owner_type::native_mobile)) +
			" AND state=" +
			std::to_string(static_cast<uint8_t>(item_custody_state::active)) +
			" ORDER BY item_uid LIMIT " + std::to_string(limits.maximum_rows + 1),
		limits.maximum_rows, bounds);
	uint64_t previous = 0;
	for (const auto &row : rows)
	{
		const auto uid = integer<uint64_t>(row[0]);
		const auto owner = integer<uint64_t>(row[1]);
		const auto found = std::lower_bound(catalog.begin(), catalog.end(), owner,
						    [](const auto &value, uint64_t id)
						    { return value.id < id; });
		require(uid && uid != UINT64_MAX && uid > previous && found != catalog.end() &&
			found->id == owner && integer<uint64_t>(row[2]) == 0 &&
			integer<uint8_t>(row[3]) ==
				static_cast<uint8_t>(item_custody_state::active));
		previous = uid;
	}
	return rows;
}
bool same_reference(const quest_mobile_native_reference &a, const quest_mobile_native_reference &b)
{
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> x{}, y{};
	return quest_mobile_native_reference_encode(a, &x) == player_snapshot_codec_result::ok &&
	       quest_mobile_native_reference_encode(b, &y) == player_snapshot_codec_result::ok &&
	       x == y;
}
}
#endif

unsigned int quest_mobile_published_world_owner::read_locked(
	MYSQL *connection, const critical_operation_id &lineage,
	std::span<const quest_mobile_native_reference> actual_held_exclusions,
	std::vector<locked_native> *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)lineage;
	(void)actual_held_exclusions;
	(void)output;
	return ENOTSUP;
#else
	try
	{
		require(connection && output && !critical_operation_id_is_zero(lineage), EINVAL);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		const economic_sql_source_limits limits{};
		require(actual_held_exclusions.size() <= limits.maximum_rows, E2BIG);
		std::map<uint64_t, quest_mobile_native_reference> exclusions;
		for (const auto &reference : actual_held_exclusions)
			require(quest_mobile_native_reference_valid(reference) &&
				exclusions.emplace(reference.mobile_instance_id, reference).second);

		auto observed = observe_catalog(connection, session);
		std::vector<uint64_t> selected;
		// Authenticate ALL relevant original inbox/root/source/mapping proofs
		// before original selected provider takes current mapping/native locks.
		// Outside-lineage current/origin/mapping values remain stable observations
		// under root's genuine global writer/admission exclusion, never locks.
		for (auto &value : observed)
		{
			const auto original = observe_origin(connection, session, value.reference);
			value.ordinary_cash_role = original.command.payload_version ==
						   NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION;
			native_mobile_birth_recovery_context recovery;
			require((value.ordinary_cash_role ?
					 native_mobile_birth_cash_role_recovery_decode(
						 original.command, original.attachment, &recovery) :
					 native_mobile_birth_recovery_decode(
						 original.command, original.attachment,
						 &recovery)) == economic_accounting_error::ok);
			const std::span<const uint8_t> receipt(
				recovery.receipt.result_payload.data(),
				recovery.receipt.result_size);
			const auto retained =
				value.ordinary_cash_role ?
					economic_sql_native_mobile_birth_ordinary_wallet_verify_retained(
						connection, original.command,
						recovery.receipt.error_code, receipt) :
					economic_sql_native_mobile_birth_verify_retained(
						connection, original.command,
						recovery.receipt.error_code, receipt);
			require(!retained, retained);
			quest_mobile_native_image born, current;
			std::vector<native_mobile_birth_item_recipe> recipes;
			native_mobile_birth_cash_role_recipe role;
			require((value.ordinary_cash_role ?
					 native_mobile_birth_cash_role_command_decode(
						 original.command, &born, &recipes, &role) :
					 native_mobile_birth_command_decode(original.command,
									    &born)) ==
					economic_accounting_error::ok &&
				(!value.ordinary_cash_role ||
				 role.role == native_mobile_birth_cash_role::ordinary_wallet) &&
				quest_mobile_native_image_decode(value.canonical, &current) ==
					player_snapshot_codec_result::ok);
			const auto origin_error = economic_sql_native_mobile_birth_observe_origin(
				connection, born.reference, &value.wallet);
			require(!origin_error, origin_error);
			require(quest_mobile_native_birth_owner::validate_progressed_origin(
				original, current, value.wallet));
			value.mapping = observe_mapping(connection, session, value.wallet);
			value.origin_hash = published_origin_digest(original.attachment);
			value.publication_revision = original.revision;
			if (critical_operation_id_equal(value.wallet.lineage, lineage))
				selected.push_back(value.id);
			active(connection, session);
		}
		const auto native_claims =
			observe_active_native_claims(connection, session, observed);
		std::vector<economic_sql_native_mobile_wallet_lifetime> wallets;
		const auto wallet_error = economic_sql_native_mobile_birth_lock_wallet_lifetimes(
			connection, lineage, &wallets);
		require(!wallet_error, wallet_error);
		require(wallets.size() == selected.size());
		std::vector<locked_native> candidate;
		candidate.reserve(wallets.size());
		size_t observed_position = 0;
		uint64_t retained_bytes = 0;
		for (size_t index = 0; index < wallets.size(); ++index)
		{
			const auto &wallet = wallets[index];
			require(wallet.native_id == selected[index]);
			while (observed_position < observed.size() &&
			       observed[observed_position].id < wallet.native_id)
				++observed_position;
			require(observed_position < observed.size() &&
				observed[observed_position].id == wallet.native_id);
			const auto &before = observed[observed_position];
			locked_native value;
			quest_mobile_native_sql_row native;
			const auto native_error =
				quest_mobile_native_sql_lock(connection, wallet.native_id, &native);
			require(!native_error, static_cast<unsigned int>(native_error));
			std::vector<uint8_t> exact;
			require(native.present && native.original_session == session &&
				native.image.cash &&
				quest_mobile_native_image_encode(native.image, &exact) ==
					player_snapshot_codec_result::ok &&
				exact == before.canonical &&
				wallet.active_native_id == wallet.native_id &&
				!wallet.mapping_revision &&
				critical_operation_id_is_zero(wallet.retiring_operation_id) &&
				wallet.native_state == quest_mobile_lifetime_state::live &&
				economic_account_key_equal(
					wallet.account,
					{ lineage, economic_account_kind::wallet,
					  before.wallet.wallet_mapping_id,
					  ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT }) &&
				wallet.balance == native.image.cash->denominations.amount &&
				wallet.native_revision == native.image.cash->revision &&
				critical_operation_id_equal(wallet.creating_operation_id,
							    before.wallet.birth_operation) &&
				critical_operation_id_equal(wallet.birth_epoch,
							    before.wallet.birth_epoch));
			// This family was authenticated before mapping/native locks. The
			// selected lifetime owner already holds its original birth inbox;
			// these exact reentrant reads acquire no new inverted inbox lock.
			const auto published_error =
				before.ordinary_cash_role ?
					quest_mobile_native_origin_sql_lock_ordinary_wallet(
						connection, native.image.reference, &value.origin) :
					quest_mobile_native_origin_sql_lock(
						connection, native.image.reference, &value.origin);
			require(!published_error, static_cast<unsigned int>(published_error));
			require(value.origin.present &&
				value.origin.original.revision == before.publication_revision &&
				published_origin_digest(value.origin.original.attachment) ==
					before.origin_hash);
			value.current = std::move(native.image);
			value.wallet = before.wallet;
			value.owner = { item_owner_type::native_mobile, wallet.native_id, 0 };
			require(exact.size() <= limits.maximum_cell_bytes - retained_bytes, E2BIG);
			retained_bytes += exact.size();
			require(value.origin.original.attachment.size() <=
					limits.maximum_cell_bytes - retained_bytes,
				E2BIG);
			retained_bytes += value.origin.original.attachment.size();
			const auto excluded = exclusions.find(wallet.native_id);
			if (excluded != exclusions.end())
				require(same_reference(excluded->second, value.current.reference));
			candidate.push_back(std::move(value));
		}
		for (const auto &[native_id, reference] : exclusions)
		{
			(void)reference;
			require(std::binary_search(selected.begin(), selected.end(), native_id));
		}
		// Reentrant selected native/origin reads preceded all NEW owner locks.
		// Every owner is the same native kind/context, therefore native-ID order
		// is the original canonical owner identity order.
		for (auto &value : candidate)
		{
			require(item_transfer_repository_lock_owner(connection, value.owner,
								    &value.owner_revision),
				errno ? static_cast<unsigned int>(errno) : EIO);
			require(value.owner_revision &&
				value.owner_revision == value.current.reference.stock_revision);
		}
		struct expected_item
		{
			size_t native;
			size_t row;
			uint64_t root;
			uint64_t parent;
		};
		std::map<uint64_t, expected_item> expected;
		std::string native_ids, uids;
		for (size_t native = 0; native < candidate.size(); ++native)
		{
			auto &value = candidate[native];
			native_ids +=
				(native_ids.empty() ? "" : ",") + std::to_string(value.owner.id);
			value.custody.reserve(value.current.items.size());
			for (size_t row = 0; row < value.current.items.size(); ++row)
			{
				const auto &literal = value.current.items[row];
				size_t root = row;
				while (value.current.items[root].parent_index !=
				       PLAYER_SNAPSHOT_NO_PARENT)
					root = value.current.items[root].parent_index;
				const uint64_t parent =
					literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
						0 :
						value.current.items[literal.parent_index].object_uid;
				require(expected.emplace(literal.object_uid,
							 expected_item{ native, row,
									value.current.items[root]
										.object_uid,
									parent })
						.second);
				require(expected.size() <= limits.maximum_rows, E2BIG);
			}
		}
		for (const auto &[uid, item] : expected)
		{
			(void)item;
			uids += (uids.empty() ? "" : ",") + std::to_string(uid);
		}
		if (!candidate.empty())
		{
			const std::string native_claim = "(owner_type=" +
							 std::to_string(static_cast<uint8_t>(
								 item_owner_type::native_mobile)) +
							 " AND owner_id IN(" + native_ids + "))";
			const std::string active_claim =
				"(state=" +
				std::to_string(static_cast<uint8_t>(item_custody_state::active)) +
				" AND (" + native_claim +
				(uids.empty() ? "" :
						" OR root_item_uid IN(" + uids +
							") OR parent_item_uid IN(" + uids + ")") +
				"))";
			const std::string predicate =
				uids.empty() ? active_claim :
					       "item_uid IN(" + uids + ") OR " + active_claim;
			const std::array<size_t, 11> bounds{
				20, 20, 20, 3, 20, 20, 20, 11, 3, 5, 1
			};
			const auto rows = query(
				connection, session,
				"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,"
				"item_revision,vnum,state,equipment_slot,(coin_payload IS NULL) FROM item_current_owner WHERE " +
					predicate + " ORDER BY item_uid LIMIT " +
					std::to_string(limits.maximum_rows + 1) + " FOR UPDATE",
				limits.maximum_rows, bounds);
			require(rows.size() == expected.size());
			uint64_t previous = 0;
			for (const auto &row : rows)
			{
				const auto uid = integer<uint64_t>(row[0]);
				require(uid && uid != UINT64_MAX && uid > previous);
				previous = uid;
				const auto found = expected.find(uid);
				require(found != expected.end());
				const auto &item = found->second;
				auto &value = candidate[item.native];
				const auto &literal = value.current.items[item.row];
				const auto revision = integer<uint64_t>(row[6]);
				require(integer<uint64_t>(row[1]) == item.root &&
					integer<uint64_t>(row[2]) == item.parent &&
					integer<uint8_t>(row[3]) ==
						static_cast<uint8_t>(value.owner.type) &&
					integer<uint64_t>(row[4]) == value.owner.id &&
					integer<uint64_t>(row[5]) == value.owner.context_id &&
					revision && revision != UINT64_MAX &&
					integer<int32_t>(row[7]) == literal.vnum &&
					integer<uint8_t>(row[8]) ==
						static_cast<uint8_t>(item_custody_state::active) &&
					integer<uint16_t>(row[9]) == literal.equipment_slot &&
					integer<uint8_t>(row[10]) == 1);
				value.custody.push_back({ uid, item.root, item.parent, value.owner,
							  revision, value.owner_revision,
							  literal.vnum,
							  item_custody_state::active });
			}
		}
		for (const auto &value : candidate)
			require(value.custody.size() == value.current.items.size());
		// Complete stable global observation recheck, without acquiring any new
		// outside native/mapping locks below selected native locks. Root retains
		// the real global writer/admission fence throughout this boundary.
		const auto after = observe_catalog(connection, session);
		require(after.size() == observed.size());
		for (size_t index = 0; index < observed.size(); ++index)
		{
			const auto &before = observed[index];
			require(after[index].id == before.id &&
				after[index].canonical == before.canonical);
			const auto original = observe_origin(connection, session, before.reference);
			require(original.revision == before.publication_revision &&
				published_origin_digest(original.attachment) ==
					before.origin_hash &&
				observe_mapping(connection, session, before.wallet) ==
					before.mapping);
		}
		require(observe_active_native_claims(connection, session, after) == native_claims);
		active(connection, session);
		std::vector<locked_native> restored;
		restored.reserve(candidate.size() - exclusions.size());
		for (auto &value : candidate)
			if (!exclusions.contains(value.current.reference.mobile_instance_id))
				restored.push_back(std::move(value));
		static_assert(std::is_nothrow_move_assignable_v<decltype(restored)>);
		*output = std::move(restored);
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
