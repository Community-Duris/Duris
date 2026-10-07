#include "item/held_retirement_recovery.h"
#include "item/held_retirement_transport.h"
#include "persistence/critical_command_repository.h"
#include "persistence/economic_sql_item_transfer_transaction.h"
#include "persistence/shop_item_runtime_payload.h"
#include "player/player_sql_transaction_cleanup.h"
#include "core/defines.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <tuple>
#include <type_traits>

#ifndef __NO_MYSQL__
namespace
{
void require(bool value, unsigned int code)
{
	if (!value)
		throw code;
}
void sql(bool value)
{
	require(value, errno ? static_cast<unsigned int>(errno) : EIO);
}
void session(MYSQL *connection, unsigned long original)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	require(connection && original && mysql_thread_id(connection) == original &&
			(connection->server_status & SERVER_STATUS_IN_TRANS) &&
			(connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
			!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
			!reconnect,
		ENOTCONN);
}
std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> read(MYSQL *connection,
							      const std::string &query)
{
	require(!mysql_real_query(connection, query.data(), query.size()),
		mysql_errno(connection) ? mysql_errno(connection) : static_cast<unsigned int>(EIO));
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(
		mysql_store_result(connection), mysql_free_result);
	require(rows != nullptr,
		mysql_errno(connection) ? mysql_errno(connection) : static_cast<unsigned int>(EIO));
	return rows;
}
void other_domains(MYSQL *connection, uint64_t uid)
{
	// These actual forest UIDs were in the complete original custody cut.
	// No synthetic selected command or late-discovered UID is introduced.
	for (const char *table :
	     { "player_items", "shopkeeper_items", "player_pet_items", "locker_items",
	       "account_locker_items", "corpse_items", "saved_items", "siege_items" })
	{
		auto rows = read(connection, "SELECT id FROM " + std::string(table) +
						     " WHERE obj_uid=" + std::to_string(uid) +
						     " LIMIT 2 FOR UPDATE");
		require(mysql_num_fields(rows.get()) == 1 &&
				mysql_num_rows(rows.get()) ==
					(!std::strcmp(table, "player_items") ? 1u : 0u),
			ESTALE);
	}
}
void forest(std::span<const player_item_snapshot> original, const item_owner_identity &owner,
	    const std::vector<item_native_quest_publication_custody> &cut,
	    std::map<uint64_t, economic_item_position> *expected)
{
	std::vector<uint64_t> roots(original.size());
	for (size_t index = 0; index < original.size(); ++index)
	{
		const auto &item = original[index];
		require(item.parent_index >= PLAYER_SNAPSHOT_NO_PARENT &&
				item.parent_index < static_cast<int32_t>(index),
			EILSEQ);
		const bool root = item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
		roots[index] = root ? item.object_uid : roots[item.parent_index];
		const auto found = std::lower_bound(cut.begin(), cut.end(), item.object_uid,
						    [](const auto &row, uint64_t uid)
						    { return row.snapshot.uid < uid; });
		require(found != cut.end() && found->snapshot.uid == item.object_uid, ESTALE);
		// Unselected revisions are genuine locked observations, not inferred
		// from the immutable literal body or today's runtime registry.
		const economic_item_position position{
			owner,
			roots[index],
			root ? 0 : original[item.parent_index].object_uid,
			found->snapshot.position.revision,
			item_custody_state::active,
			static_cast<uint16_t>(item.equipment_slot)
		};
		require(position.revision && position.revision != UINT64_MAX &&
				found->vnum == item.vnum &&
				economic_item_position_equal(found->snapshot.position, position) &&
				expected->emplace(item.object_uid, position).second,
			ESTALE);
	}
}
bool receipt_equal(const critical_apply_result &actual, const critical_completion &original)
{
	const bool success = original.outcome == critical_apply_outcome::applied ||
			     original.outcome == critical_apply_outcome::already_applied;
	return (success ? (actual.outcome == critical_apply_outcome::applied ||
			   actual.outcome == critical_apply_outcome::already_applied) :
			  actual.outcome == critical_apply_outcome::terminal_failure) &&
	       actual.error_code == original.error_code &&
	       actual.failure_stage == original.failure_stage &&
	       actual.durable_revision == original.durable_revision &&
	       actual.result_size == original.result_size &&
	       actual.result_payload == original.result_payload;
}
void projection(MYSQL *connection, const critical_command &command,
		const critical_completion &completion, const held_retirement_recovery &captured,
		held_retirement_current_observation *output)
{
	const unsigned long original_session = mysql_thread_id(connection);
	session(connection, original_session);
	item_transfer_payload payload{};
	std::vector<uint8_t> encoded;
	require(held_retirement_transport_command(command) &&
			held_retirement_command_identity(command, &payload) &&
			held_retirement_recovery_encode(command, captured, &encoded) &&
			critical_completion_disposition_valid(completion) &&
			completion.operation_id.bytes == command.operation_id.bytes,
		EILSEQ);
	const bool refused = completion.disposition ==
			     critical_completion_disposition::never_admitted;
	const bool success = !refused &&
			     (completion.outcome == critical_apply_outcome::applied ||
			      completion.outcome == critical_apply_outcome::already_applied);
	if (refused)
	{
		require(!captured.receipt.present && !captured.physical_stage, EILSEQ);
		const auto error =
			critical_command_repository_verify_held_retirement_refusal_in_transaction(
				connection, command, completion);
		require(!error, error);
	}
	else
	{
		require(success ||
				(completion.outcome == critical_apply_outcome::terminal_failure &&
				 completion.error_code),
			EAGAIN);
		require(!captured.receipt.present || held_retirement_recovery_receipt_matches(
							     captured.receipt, completion),
			EILSEQ);
		const auto authentic =
			critical_command_repository_verify_held_retirement_in_transaction(
				connection, command);
		require(receipt_equal(authentic, completion),
			authentic.error_code ? authentic.error_code :
					       static_cast<unsigned int>(EILSEQ));
	}
	// All original receipt/source proof precedes participant locks. No late
	// inbox lock or current-epoch re-admission is taken by this observation.
	uint64_t save_revision = 0;
	const auto save_error = economic_sql_held_retirement_lock_saved_revision(
		connection, command, captured, &save_revision);
	require(!save_error, save_error);
	std::array<item_owner_identity, 2> owners{ payload.from_owner, payload.to_owner };
	std::sort(owners.begin(), owners.end(),
		  [](const auto &a, const auto &b) {
			  return std::tie(a.type, a.id, a.context_id) <
				 std::tie(b.type, b.id, b.context_id);
		  });
	uint64_t from_revision = 0, to_revision = 0;
	for (const auto &owner : owners)
	{
		uint64_t observed = 0;
		sql(item_transfer_repository_lock_owner(connection, owner, &observed));
		(item_owner_identity_equal(owner, payload.from_owner) ? from_revision :
									to_revision) = observed;
	}
	require(payload.expected_from_revision != UINT64_MAX &&
			payload.expected_to_revision != UINT64_MAX &&
			from_revision == payload.expected_from_revision + (success ? 1u : 0u) &&
			to_revision >= payload.expected_to_revision + (success ? 1u : 0u),
		ESTALE);
	std::vector<item_native_quest_publication_custody> cut;
	sql(item_transfer_repository_lock_held_retirement_publication_custody(connection, command,
									      captured, &cut));
	const auto &wanted = success ? captured.after : captured.before;
	std::map<uint64_t, economic_item_position> expected;
	forest(wanted, payload.from_owner, cut, &expected);
	if (success)
	{
		const auto &entry = payload.items[0];
		const economic_item_position destroyed{ payload.to_owner,
							entry.item_uid,
							0,
							entry.expected_item_revision + 1,
							item_custody_state::destroyed,
							0 };
		require(expected.emplace(entry.item_uid, destroyed).second, EILSEQ);
	}
	require(expected.size() == cut.size(), ESTALE);
	const item_native_quest_publication_custody *selected = nullptr;
	for (const auto &row : cut)
	{
		const auto found = expected.find(row.snapshot.uid);
		require(found != expected.end() &&
				economic_item_position_equal(found->second, row.snapshot.position),
			ESTALE);
		if (row.snapshot.uid == payload.selected_item_uid)
			selected = &row;
	}
	require(selected && selected->vnum == payload.items[0].vnum &&
			selected->snapshot.position.root_uid == payload.selected_item_uid &&
			!selected->snapshot.position.parent_uid &&
			selected->snapshot.position.revision ==
				payload.items[0].expected_item_revision + (success ? 1u : 0u) &&
			selected->snapshot.position.equipment_slot == (success ? 0 : HOLD + 1),
		ESTALE);
	// A selected pick cannot acquire a coin sidecar. Re-read only its already
	// globally locked UID; the cut itself intentionally exposes foreign rows.
	auto coin = read(connection,
			 "SELECT coin_payload IS NULL FROM item_current_owner WHERE item_uid=" +
				 std::to_string(payload.selected_item_uid) + " LIMIT 2 FOR UPDATE");
	MYSQL_ROW coin_row = mysql_fetch_row(coin.get());
	require(mysql_num_fields(coin.get()) == 1 && mysql_num_rows(coin.get()) == 1 && coin_row &&
			coin_row[0] && !std::strcmp(coin_row[0], "1"),
		ESTALE);
	shop_item_runtime_image image;
	sql(shop_item_runtime_lock_player_image(connection, payload.from_owner.id, wanted, &image));
	const auto domain_error = economic_sql_held_retirement_lock_selected_domains(
		connection, command, captured, success);
	require(!domain_error, domain_error);
	for (const auto &row : cut)
		if (row.snapshot.uid != payload.selected_item_uid)
			other_domains(connection, row.snapshot.uid);
	session(connection, original_session);
	held_retirement_current_observation candidate;
	candidate.save_revision = save_revision;
	// The installed physical reader proved this complete canonical body against
	// actual SQL strings/policies/sidecars, topology and equipment, including empty.
	candidate.items = wanted;
	const auto &position = selected->snapshot.position;
	candidate.selected = { selected->snapshot.uid, position.root_uid,
			       position.parent_uid,    position.owner,
			       position.revision,      success ? to_revision : from_revision,
			       selected->vnum,	       position.state };
	candidate.from_owner_revision = from_revision;
	candidate.to_owner_revision = to_revision;
	*output = std::move(candidate);
}
}
#endif

bool held_retirement_repository_observe_current(const critical_command &command,
						const critical_completion &completion,
						const held_retirement_recovery &captured,
						held_retirement_current_observation *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)completion;
	(void)captured;
	(void)output;
	return false;
#else
	if (!output)
		return false;
	try
	{
		// Same trusted publication-pool pattern as the native quest owner. No
		// replacement handle or second session can stand in for original proof.
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		held_retirement_current_observation candidate;
		bool proven = false;
		try
		{
			if (!mysql_real_query(connection, "START TRANSACTION", 17))
			{
				projection(connection, command, completion, captured, &candidate);
				proven = transaction.same_session();
			}
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified ||
		    !transaction.same_session())
			return false;
		candidate.rollback_confirmed = true;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}
