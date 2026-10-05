#include "economy/collector_transaction.h"
#include "economy/collector_accounting.h"
#include "economy/currency_transaction.h"
#include "economy/collector_runtime.h"
#include "item/item_ownership_runtime.h"
#include "player/player_save_pipeline.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "persistence/persistence_mode.h"
#ifndef __NO_MYSQL__
#include "economy/collector_repository.h"
#include "persistence/critical_command_repository.h"
#include "persistence/economic_sql_collector_transaction.h"
#include "player/player_sql_transaction_cleanup.h"
#endif
#include <openssl/sha.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <unordered_set>

extern P_obj object_list;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern const int top_of_world;

namespace
{
bool committed(critical_apply_outcome outcome)
{
	return outcome == critical_apply_outcome::applied ||
	       outcome == critical_apply_outcome::already_applied;
}
bool same_seal(const critical_completion &a, const critical_completion &b) noexcept
{
	return a.operation_id.bytes == b.operation_id.bytes && a.disposition == b.disposition &&
	       ((committed(a.outcome) && committed(b.outcome)) || a.outcome == b.outcome) &&
	       a.error_code == b.error_code && a.failure_stage == b.failure_stage &&
	       a.durable_revision == b.durable_revision && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload;
}
#ifndef __NO_MYSQL__
bool actor_matches(P_char actor, const collector_command_payload &payload,
		   uint64_t runtime) noexcept
{
	const char *account = actor ? get_account_name_safe(actor) : nullptr;
	return actor && IS_PC(actor) && actor->only.pc && actor->runtime_id == runtime &&
	       GET_PID(actor) > 0 && static_cast<uint32_t>(GET_PID(actor)) == payload.actor_pid &&
	       account && !strcmp(account, payload.account_name.data()) &&
	       actor->player.racewar == payload.racewar;
}

// Preserve the existing descriptor first-match lookup, but validate its whole
// chain before absence can select the offline path. No pointer survives a pulse.
bool resolve_actor(uint32_t pid, P_char *out)
{
	std::unordered_set<P_desc> seen;
	P_char found = nullptr;
	for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
	{
		if (seen.size() >= 1000000 || !seen.insert(descriptor).second)
			return false;
		if (!found && STATE(descriptor) == CON_PLAYING && descriptor->character &&
		    IS_PC(descriptor->character) &&
		    GET_PID(descriptor->character) == static_cast<int>(pid))
			found = descriptor->character;
	}
	*out = found;
	return true;
}

// Complete bounded physical link census for the singleton, including aliases
// whose stale location field would otherwise hide them. Never changes the world.
bool singleton_census(uint64_t uid, P_char actor, bool rejection, P_obj *selected,
		      uint32_t absent_pid = 0)
{
	constexpr size_t limit = 1000000;
	size_t visits = 0, carried_links = 0;
	std::unordered_set<P_obj> objects;
	P_obj match = nullptr;
	for (P_obj object = object_list; object; object = object->next)
	{
		if (++visits > limit || !objects.insert(object).second)
			return false;
		if (object->obj_uid == uid)
		{
			if (match)
				return false;
			match = object;
		}
	}
	auto links = [&](P_obj head, P_char carrier)
	{
		std::unordered_set<P_obj> seen;
		for (P_obj object = head; object; object = object->next_content)
		{
			if (++visits > limit || !seen.insert(object).second ||
			    !objects.count(object))
				return false;
			if (object->obj_uid == uid &&
			    (rejection || object != match || !carrier || carrier != actor ||
			     object->loc_p != LOC_CARRIED || object->loc.carrying != actor ||
			     ++carried_links != 1))
				return false;
		}
		return true;
	};
	for (P_obj object : objects)
		if (!links(object->contains, nullptr))
			return false;
	if (!world || top_of_world < 0)
		return false;
	for (int room = 0; room <= top_of_world; ++room)
		if (++visits > limit || !links(world[room].contents, nullptr))
			return false;
	std::unordered_set<P_char> characters, scanned_bodies;
	std::vector<P_char> bodies;
	auto scan_body = [&](P_char character)
	{
		if (!character || !scanned_bodies.insert(character).second)
			return true;
		if (absent_pid && IS_PC(character) &&
		    GET_PID(character) == static_cast<int>(absent_pid))
			return false;
		if (++visits > limit || !links(character->carrying, character))
			return false;
		bodies.push_back(character);
		for (size_t slot = 0; slot < MAX_WEAR; ++slot)
		{
			const P_obj object = character->equipment[slot];
			if (!object)
				continue;
			if (++visits > limit || !objects.count(object) || object->obj_uid == uid)
				return false;
		}
		return true;
	};
	for (P_char character = character_list; character; character = character->next)
		if (!characters.insert(character).second || !scan_body(character))
			return false;
	// Descriptor current/original bodies and morph owners need not be on the
	// global character list. Scan each body once so duplicate references do not
	// manufacture an extra carrying link, but distinct-body aliases still refuse.
	std::unordered_set<P_desc> descriptors;
	for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
		if (++visits > limit || !descriptors.insert(descriptor).second ||
		    !scan_body(descriptor->character) || !scan_body(descriptor->original))
			return false;
	if (!scan_body(actor))
		return false;
	for (size_t index = 0; index < bodies.size(); ++index)
		if (!scan_body(GET_PLYR(bodies[index])))
			return false;
	if (rejection ? match != nullptr :
			(match && (carried_links != 1 || match->contains ||
				   match->loc_p != LOC_CARRIED || match->loc.carrying != actor)))
		return false;
	*selected = match;
	return true;
}

bool balances_match(P_char actor, const collector_purchase_current_projection &current) noexcept
{
	return actor && actor->only.pc &&
	       actor->only.pc->wallet_revision == current.result.wallet_revision &&
	       actor->only.pc->bank_revision == current.result.bank_revision &&
	       current.result.wallet.amount ==
		       std::array<int64_t, 4>{ GET_COPPER(actor), GET_SILVER(actor),
					       GET_GOLD(actor), GET_PLATINUM(actor) } &&
	       current.result.bank.amount == std::array<int64_t, 4>{ GET_BALANCE_COPPER(actor),
								     GET_BALANCE_SILVER(actor),
								     GET_BALANCE_GOLD(actor),
								     GET_BALANCE_PLATINUM(actor) };
}

bool registry_matches(const collector_command_payload &payload,
		      const collector_purchase_current_projection &current, bool rejected,
		      bool allow_prior)
{
	std::vector<item_ownership_runtime_entry> entries;
	item_ownership_runtime_entry by_uid;
	if (item_ownership_runtime_lookup(payload.selected_item_uid, &by_uid) &&
	    by_uid.root_item_uid != payload.selected_item_uid)
		return false;
	if (!item_ownership_runtime_snapshot_root(payload.selected_item_uid, 2, &entries))
		return false;
	if (allow_prior && entries.empty())
		return !rejected;
	if (entries.size() != 1)
		return false;
	const auto &entry = entries[0];
	const bool prior = !rejected && allow_prior &&
			   item_owner_identity_equal(entry.owner, payload.from_owner);
	const auto owner = rejected || prior ? payload.from_owner : payload.to_owner;
	const uint64_t revision = prior ? payload.items[0].expected_item_revision :
					  current.result.entry.item_revision;
	const uint64_t ceiling = rejected || prior ? current.result.from_owner_revision :
						     current.result.to_owner_revision;
	uint64_t cached = 0;
	return entry.item_uid == payload.selected_item_uid &&
	       entry.root_item_uid == entry.item_uid && !entry.parent_item_uid &&
	       item_owner_identity_equal(entry.owner, owner) && entry.item_revision == revision &&
	       entry.vnum == payload.items[0].vnum && entry.state == item_custody_state::active &&
	       entry.owner_revision <= ceiling &&
	       item_ownership_runtime_peek_owner_revision(owner, &cached) &&
	       cached >= entry.owner_revision && cached <= ceiling;
}

bool catalog_matches(const collector_purchase_current_projection &current)
{
	collector::record cached{};
	std::array<uint8_t, collector::encoded_record_bytes> expected{}, actual{};
	return collector_runtime_catalog_revision() <= current.result.catalog_revision &&
	       collector_runtime_find(current.result.entry.listing, &cached) &&
	       collector::record_encode(current.result.entry, &expected) ==
		       collector::codec_result::ok &&
	       collector::record_encode(cached, &actual) == collector::codec_result::ok &&
	       actual == expected;
}

// Only nonphysical projections are reconciled here, under the current native
// proof. This never constructs an object or substitutes a projection for a seal.
bool publish_offline_projection(const collector_command_payload &payload,
				const collector_purchase_current_projection &current, bool rejected)
{
	std::vector<item_ownership_runtime_entry> entries;
	item_ownership_runtime_entry by_uid;
	if ((item_ownership_runtime_lookup(payload.selected_item_uid, &by_uid) &&
	     by_uid.root_item_uid != payload.selected_item_uid) ||
	    !item_ownership_runtime_snapshot_root(payload.selected_item_uid, 2, &entries) ||
	    entries.size() > 1)
		return false;
	const auto owner = rejected ? payload.from_owner : payload.to_owner;
	const uint64_t ceiling = rejected ? current.result.from_owner_revision :
					    current.result.to_owner_revision;
	if (!entries.empty())
	{
		const auto &entry = entries[0];
		const bool prior = !rejected &&
				   item_owner_identity_equal(entry.owner, payload.from_owner) &&
				   entry.item_revision == payload.items[0].expected_item_revision;
		const uint64_t previous_ceiling = prior ? current.result.from_owner_revision :
							  ceiling;
		if (entry.item_uid != payload.selected_item_uid ||
		    entry.root_item_uid != entry.item_uid || entry.parent_item_uid ||
		    entry.vnum != payload.items[0].vnum ||
		    entry.state != item_custody_state::active ||
		    (!prior && !item_owner_identity_equal(entry.owner, owner)) ||
		    entry.item_revision > current.result.entry.item_revision ||
		    entry.owner_revision > previous_ceiling)
			return false;
	}
	// Rejection has no cache mutation to replay: require the current held record
	// supplied by the existing authoritative catalog loader, not a fake action.
	if (rejected && !catalog_matches(current))
		return false;
	const item_ownership_runtime_entry desired = { payload.selected_item_uid,
						       payload.selected_item_uid,
						       0,
						       owner,
						       current.result.entry.item_revision,
						       ceiling,
						       payload.items[0].vnum,
						       item_custody_state::active };
	if (!item_ownership_runtime_hydrate_many_atomic(&desired, 1) ||
	    !item_ownership_runtime_hydrate_owner(payload.from_owner,
						  current.result.from_owner_revision) ||
	    (!rejected && !collector_runtime_publish(current.result)))
		return false;
	return registry_matches(payload, current, rejected, false) && catalog_matches(current);
}

bool literal_matches(P_obj selected, const collector_command_payload &payload, uint32_t row)
{
	if (!selected || selected->db_item_id < 0 ||
	    static_cast<uint32_t>(selected->db_item_id) != row)
		return false;
	std::vector<player_item_snapshot> actual;
	size_t estimated = 0;
	std::vector<uint8_t> encoded;
	return player_item_snapshot_tree_capture(selected, &actual, &estimated) ==
		       player_snapshot_capture_result::ok &&
	       actual.size() == 1 &&
	       player_item_snapshot_list_encode(actual, &encoded) ==
		       player_snapshot_codec_result::ok &&
	       encoded.size() == payload.item_blob_size &&
	       std::equal(encoded.begin(), encoded.end(), payload.item_blob.begin());
}
#endif
}

// Defined only in this implementation. Its pipeline friendship is the sole
// route to guarded ACK; callers cannot supply arbitrary successful callbacks.
class collector_purchase_publication_owner final
{
	struct attempt_context
	{
		uint64_t runtime;
		collector_purchase_publication_state &state;
		collector_purchase_effect_fn effect;
	};
	static bool native_publish(const critical_command &command,
				   const critical_completion &sealed, void *opaque) noexcept
	{
#ifdef __NO_MYSQL__
		(void)command;
		(void)sealed;
		(void)opaque;
		return false;
#else
		auto &attempt = *static_cast<attempt_context *>(opaque);
		try
		{
			collector_command_payload payload{};
			collector_command_result original{};
			if (!collector_command_decode_payload(command, &payload) ||
			    !collector_command_decode_result(sealed.result_payload.data(),
							     sealed.result_size, &original) ||
			    attempt.state.receipt_conflict || !attempt.effect)
				return false;
			MYSQL *connection = sql_pool_acquire();
			player_sql_pool_lease lease(connection);
			if (!connection || player_sql_idle_error(connection))
				return false;
			player_sql_cleanup cleanup;
			player_sql_transaction_cleanup transaction(connection, cleanup);
			transaction.starting();
			bool proven = false;
			try
			{
				if (mysql_real_query(connection, "START TRANSACTION", 17))
					throw EIO;
				economic_sql_collector_context authority;
				if (economic_sql_collector_lock(connection, command, &authority))
					throw EAGAIN;
				collector_purchase_current_projection current;
				if (!collector_repository_read_purchase_projection(
					    connection, command, original, sealed.error_code,
					    &current) ||
				    current.bank_id != authority.bank_id)
					throw EAGAIN;
				const auto receipt =
					critical_command_repository_verify_collector_purchase_in_transaction(
						connection, command);
				if ((committed(sealed.outcome) ?
					     receipt.outcome !=
						     critical_apply_outcome::already_applied :
					     receipt.outcome !=
						     critical_apply_outcome::terminal_failure) ||
				    receipt.error_code != sealed.error_code ||
				    receipt.failure_stage != sealed.failure_stage ||
				    receipt.durable_revision != sealed.durable_revision ||
				    receipt.result_size != sealed.result_size ||
				    receipt.result_payload != sealed.result_payload ||
				    !transaction.same_session() ||
				    mysql_thread_id(connection) != authority.session_id ||
				    !(connection->server_status & SERVER_STATUS_IN_TRANS))
					throw EAGAIN;
				P_char actor = nullptr;
				if (!resolve_actor(payload.actor_pid, &actor))
					throw EAGAIN;
				if (!actor)
				{
					P_obj selected = nullptr;
					if ((attempt.state.materializer_started &&
					     !attempt.state.materializer_returned) ||
					    !singleton_census(payload.selected_item_uid, nullptr,
							      true, &selected, payload.actor_pid) ||
					    !publish_offline_projection(payload, current,
									sealed.error_code != 0) ||
					    attempt.state.receipt_conflict ||
					    !singleton_census(payload.selected_item_uid, nullptr,
							      true, &selected, payload.actor_pid))
						throw EAGAIN;
					// The exact native rows retain the future load payload; no live
					// body or physical item is created, and no service callback runs.
					proven = !attempt.state.receipt_conflict &&
						 transaction.same_session() &&
						 mysql_thread_id(connection) ==
							 authority.session_id &&
						 (connection->server_status &
						  SERVER_STATUS_IN_TRANS);
				}
				else
				{
					if (!actor_matches(actor, payload, attempt.runtime))
						throw EAGAIN;
					P_obj selected = nullptr;
					if (!singleton_census(payload.selected_item_uid, actor,
							      sealed.error_code != 0, &selected) ||
					    !registry_matches(payload, current,
							      sealed.error_code != 0, true))
						throw EAGAIN;
					if (committed(sealed.outcome))
					{
						if (!attempt.effect(actor, current.result, payload,
								    attempt.state))
							throw EAGAIN;
						actor = find_player_by_pid(payload.actor_pid);
						if (!actor_matches(actor, payload,
								   attempt.runtime) ||
						    attempt.state.receipt_conflict ||
						    !singleton_census(payload.selected_item_uid,
								      actor, false, &selected) ||
						    !literal_matches(
							    selected, payload,
							    current.result.materialized_item_id) ||
						    !registry_matches(payload, current, false,
								      false))
							throw EAGAIN;
					}
					else
					{
						// A verified no-effect rejection does not invoke materialization.
						if (!currency_transaction_publish_balances(
							    actor, payload.account_name.data(),
							    payload.racewar, current.result.wallet,
							    current.result.bank,
							    current.result.wallet_revision,
							    current.result.bank_revision))
							throw EAGAIN;
					}
					actor = find_player_by_pid(payload.actor_pid);
					proven = actor_matches(actor, payload, attempt.runtime) &&
						 balances_match(actor, current) &&
						 !attempt.state.receipt_conflict &&
						 transaction.same_session() &&
						 mysql_thread_id(connection) ==
							 authority.session_id &&
						 (connection->server_status &
						  SERVER_STATUS_IN_TRANS);
				}
			}
			catch (...)
			{
				proven = false;
			}
			transaction.finish();
			if (!cleanup.rollback_confirmed || cleanup.cleanup_error ||
			    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
				return false;
			lease.reuse(cleanup);
			return proven && !attempt.state.receipt_conflict;
		}
		catch (...)
		{
			return false;
		}
#endif
	}

    public:
	static bool publish(const critical_command &command, const critical_completion &sealed,
			    uint64_t runtime, collector_purchase_publication_state &state,
			    collector_purchase_effect_fn effect) noexcept
	{
		try
		{
			if (!nevent_is_game_thread() ||
			    persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY ||
			    !critical_command_envelope_valid(command) ||
			    !command.publication_required ||
			    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
			    command.operation_id.bytes != sealed.operation_id.bytes ||
			    !critical_completion_disposition_valid(sealed) ||
			    sealed.failure_stage != critical_failure_stage::none)
				return false;
			economic_frozen_intent intent;
			collector_command_payload payload{};
			collector::record listing;
			economic_account_key wallet, bank;
			if (collector_purchase_accounting_decode(command, &intent, &payload,
								 &listing, &wallet, &bank) !=
			    economic_accounting_error::ok)
				return false;
			const bool never_admitted = sealed.disposition ==
						    critical_completion_disposition::never_admitted;
			if (!never_admitted)
			{
				collector_command_result result{};
				std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> encoded{};
				if (sealed.result_size != encoded.size() ||
				    (!committed(sealed.outcome) &&
				     sealed.outcome != critical_apply_outcome::terminal_failure) ||
				    (committed(sealed.outcome) ? sealed.error_code != 0 :
								 sealed.error_code == 0) ||
				    !collector_command_decode_result(sealed.result_payload.data(),
								     sealed.result_size, &result) ||
				    !collector_command_encode_result(result, &encoded) ||
				    !std::equal(encoded.begin(), encoded.end(),
						sealed.result_payload.begin()) ||
				    std::any_of(sealed.result_payload.begin() + encoded.size(),
						sealed.result_payload.end(),
						[](uint8_t byte) { return byte != 0; }))
					return false;
			}
			std::vector<uint8_t> canonical;
			std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
			if (critical_command_encode(command, &canonical) !=
			    critical_command_codec_result::ok)
				return false;
			SHA256(canonical.data(), canonical.size(), digest.data());
			if (state.bound_ && (state.operation_.bytes != command.operation_id.bytes ||
					     state.command_digest_ != digest ||
					     !same_seal(state.original_, sealed)))
				return false;
			if (!state.bound_)
			{
				state.operation_ = command.operation_id;
				state.command_digest_ = digest;
				state.original_ = sealed;
				state.bound_ = true;
			}
			if (state.receipt_conflict)
				return false;
			attempt_context context{ runtime, state, effect };
			return player_save_restored_publication_owner::publish_collector(
				command, sealed, native_publish, &context);
		}
		catch (...)
		{
			return false;
		}
	}
};

critical_submit_result collector_purchase_submit_for_publication(critical_command command)
{
#ifdef __NO_MYSQL__
	(void)command;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread() || persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return critical_submit_result::unavailable;
	return collector_purchase_submit_owned(std::move(command));
#endif
}

bool collector_purchase_publication_attempt(const critical_command &command,
					    const critical_completion &sealed, uint64_t runtime,
					    collector_purchase_publication_state &state,
					    collector_purchase_effect_fn effect)
{
	return collector_purchase_publication_owner::publish(command, sealed, runtime, state,
							     effect);
}
