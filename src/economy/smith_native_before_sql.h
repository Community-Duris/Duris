#ifndef SMITH_NATIVE_BEFORE_SQL_H
#define SMITH_NATIVE_BEFORE_SQL_H

#include "core/prototypes.h"
#include "economy/smith_native_producer.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
#include "persistence/quest_mobile_native_sql.h"
#include "persistence/sql_room_item_payload.h"
#include "world/quest_mobile_native.h"
#include "world/quest_mobile_native_binding.h"

#include <type_traits>
#include <utility>

// Original observations only. In particular, these values neither prove an
// admitted source nor permit a factory, SQL write, physical publication or ACK.
struct smith_native_persisted_before
{
	bool captured = false;
	uint64_t season_epoch = 0;
	quest_mobile_native_image native_before;
	native_mobile_wallet_origin native_origin;

    private:
	friend class smith_native_before_sql;
	friend class smith_native_compound_owner;
	const smith_native_original_selection *original_ = nullptr;
};

class smith_native_before_sql final
{
    private:
	friend class smith_native_compound_owner;

	// Borrow the ORIGINAL reconnect-disabled IN_TRANS SQL session, while the
	// root owns lifecycle/writer exclusion and the real indexed generations.
	// Observe season/original birth before current native-image locks. All
	// calls are SELECT-only. The root must explicitly finish and prove the
	// original rollback/idle cleanup BEFORE permitting factory/native effects.
	// This method never starts, commits, rolls back or replaces that session.
	static bool observe(MYSQL *connection, const smith_native_original_selection &selection,
			    smith_native_persisted_before *output) noexcept
	{
#ifdef __NO_MYSQL__
		(void)connection;
		(void)selection;
		(void)output;
		return false;
#else
		if (!connection || !output || !selection.captured || !nevent_is_game_thread() ||
		    !economic_gameplay_authority::active_regular_sql() ||
		    (output->captured && output->original_ != &selection))
			return false;
		try
		{
			const auto session = mysql_thread_id(connection);
			const auto same_session = [&]()
			{
				using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
				flag reconnect = true;
				return session && mysql_thread_id(connection) == session &&
				       (connection->server_status & SERVER_STATUS_IN_TRANS) &&
				       (connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
				       !mysql_get_option(connection, MYSQL_OPT_RECONNECT,
							 &reconnect) &&
				       !reconnect;
			};
			if (!same_session())
				return false;
			const auto &expected = selection.terms.native_before;
			auto *mobile = find_character_by_runtime_id(selection.mobile_runtime);
			if (!mobile)
				return false;
			quest_mobile_native_cash_reference current;
			if (!quest_mobile_native_cash_reference_copy(
				    mobile, selection.mobile_runtime, &current))
				return false;
			std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>
				expected_reference{}, current_reference{};
			if (quest_mobile_native_reference_encode(expected.reference,
								 &expected_reference) !=
				    player_snapshot_codec_result::ok ||
			    quest_mobile_native_reference_encode(current.reference,
								 &current_reference) !=
				    player_snapshot_codec_result::ok ||
			    expected_reference != current_reference ||
			    expected.lineage.bytes != current.lineage.bytes ||
			    expected.birth_epoch.bytes != current.birth_epoch.bytes ||
			    expected.wallet_mapping_id != current.wallet_mapping_id ||
			    expected.cash_revision != current.cash_revision ||
			    expected.denominations != current.denominations ||
			    expected.lineage.bytes != selection.admission_mapping.lineage.bytes)
				return false;
			smith_native_persisted_before candidate;
			if (!sql_room_item_payload_lock_season(connection,
							       &candidate.season_epoch) ||
			    !candidate.season_epoch || !same_session() ||
			    economic_sql_native_mobile_birth_observe_origin(
				    connection, expected.reference, &candidate.native_origin) ||
			    !same_session())
				return false;
			const auto &origin = candidate.native_origin;
			if (origin.birth_operation.bytes !=
				    expected.reference.birth_operation.bytes ||
			    origin.mobile_instance_id != expected.reference.mobile_instance_id ||
			    origin.lineage.bytes != expected.lineage.bytes ||
			    origin.birth_epoch.bytes != expected.birth_epoch.bytes ||
			    origin.wallet_mapping_id != expected.wallet_mapping_id)
				return false;
			quest_mobile_native_sql_row row;
			if (quest_mobile_native_sql_lock(
				    connection, expected.reference.mobile_instance_id, &row) ||
			    !same_session() || row.original_session != session || !row.present ||
			    row.image.state != quest_mobile_lifetime_state::live ||
			    !row.image.cash || row.image.cash->revision != expected.cash_revision ||
			    row.image.cash->denominations.amount != expected.denominations)
				return false;
			std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>
				persisted_reference{};
			if (quest_mobile_native_reference_encode(row.image.reference,
								 &persisted_reference) !=
				    player_snapshot_codec_result::ok ||
			    persisted_reference != expected_reference)
				return false;
			// Capture consumes the genuine persisted historical transition and
			// cash revision. Neither is inferred from runtime/current balance.
			quest_mobile_native_image actual;
			if (quest_mobile_native_capture(
				    mobile, expected.reference, quest_mobile_lifetime_state::live,
				    row.image.last_transition_operation, row.image.cash->revision,
				    &actual) != player_snapshot_capture_result::ok)
				return false;
			std::vector<uint8_t> persisted, physical, selected, actual_items;
			if (quest_mobile_native_image_encode(row.image, &persisted) !=
				    player_snapshot_codec_result::ok ||
			    quest_mobile_native_image_encode(actual, &physical) !=
				    player_snapshot_codec_result::ok ||
			    persisted != physical ||
			    player_item_snapshot_list_encode(selection.full_native_items,
							     &selected) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode(actual.items, &actual_items) !=
				    player_snapshot_codec_result::ok ||
			    selected != actual_items || !same_session())
				return false;
			if (output->captured)
			{
				const auto &held = output->native_origin;
				std::vector<uint8_t> frozen;
				return output->season_epoch == candidate.season_epoch &&
				       held.birth_operation.bytes == origin.birth_operation.bytes &&
				       held.mobile_instance_id == origin.mobile_instance_id &&
				       held.lineage.bytes == origin.lineage.bytes &&
				       held.birth_epoch.bytes == origin.birth_epoch.bytes &&
				       held.wallet_mapping_id == origin.wallet_mapping_id &&
				       quest_mobile_native_image_encode(output->native_before,
									&frozen) ==
					       player_snapshot_codec_result::ok &&
				       frozen == persisted;
			}
			candidate.native_before = std::move(row.image);
			candidate.original_ = &selection;
			candidate.captured = true;
			*output = std::move(candidate);
			return true;
		}
		catch (...)
		{
			return false;
		}
#endif
	}
};

#endif
