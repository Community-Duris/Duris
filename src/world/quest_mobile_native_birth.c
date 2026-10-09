#include "economy/native_mobile_birth_recovery.h"
#include "world/quest_mobile_native_birth.h"
#include "world/zone_reset_item_owner.h"
#include "world/db.h"
#include "world/native_mobile_birth_artifact.h"
#include "world/native_mobile_birth_procedure.h"
#include "world/native_mobile_birth_reset_tail.h"
#include "world/events.h"
#include <cerrno>
#include "world/handler.h"
#include "world/object_template.h"
#include "world/quest_mobile_native.h"
#include "world/world_singletons.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/native_mobile_birth_command.h"
#include "economy/native_mobile_birth_result.h"
#include "economy/native_mobile_birth_cash_role_result.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "flatfile/flatfile_shopkeeper_capture.h"
#include "economy/shop_trade_runtime.h"
#include "item/item_uid_allocator.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "item/enhance.h"
#include "item/encumbrance_policy.h"
#include "player/player_save_pipeline.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "classes/npc_alchemist.h"
#include "world/vnum.obj.h"
#ifndef __NO_MYSQL__
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
#include "persistence/quest_mobile_native_origin_sql.h"
#include "player/player_sql_transaction_cleanup.h"
#endif
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstring>
#include <memory>
#include <vector>

extern index_data *mob_index;
extern index_data *obj_index;
extern zone_data *zone_table;
extern int top_of_zone_table;
extern int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
extern P_room world;
extern P_char character_list;
extern P_obj object_list;
extern struct shop_data *shop_index;
extern int number_of_shops;
extern void apply_zone_modifier(P_char);

namespace
{

bool ordinary_wallet_command(const critical_command &command) noexcept
{
	return command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION;
}
// Conservative codec choice only. Malformed NMB4+SHOP must reach the full
// shared validator and must never fall back to the ordinary recovery family.
bool shared_shop_command(const critical_command &command) noexcept
{
	return command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
	       std::any_of(command.keys.begin(), command.keys.end(), [](const auto &key)
			   { return key.type == critical_entity_type::shopkeeper; });
}
economic_accounting_error birth_recovery_encode(
	const critical_command &command, const native_mobile_birth_recovery_context &context,
	std::vector<uint8_t> *output, const std::vector<uint8_t> *checkpoint = nullptr) noexcept
{
	if (shared_shop_command(command))
	{
		if (!checkpoint || checkpoint->empty())
			return economic_accounting_error::corrupt_evidence;
		try
		{
			native_mobile_birth_shared_shop_recovery_context shared;
			shared.progress = context;
			shared.original_checkpoint = *checkpoint;
			return native_mobile_birth_shared_shop_recovery_encode(command, shared,
									       output);
		}
		catch (...)
		{
			return economic_accounting_error::capacity;
		}
	}
	return ordinary_wallet_command(command) ?
		       native_mobile_birth_cash_role_recovery_encode(command, context, output) :
		       native_mobile_birth_recovery_encode(command, context, output);
}
economic_accounting_error birth_recovery_decode(const critical_command &command,
						std::span<const uint8_t> attachment,
						native_mobile_birth_recovery_context *output,
						std::vector<uint8_t> *checkpoint = nullptr) noexcept
{
	if (shared_shop_command(command))
	{
		if (!output || !checkpoint)
			return economic_accounting_error::corrupt_evidence;
		native_mobile_birth_shared_shop_recovery_context shared;
		const auto error = native_mobile_birth_shared_shop_recovery_decode(
			command, attachment, &shared);
		if (error != economic_accounting_error::ok)
			return error;
		*output = std::move(shared.progress);
		*checkpoint = std::move(shared.original_checkpoint);
		return economic_accounting_error::ok;
	}
	return ordinary_wallet_command(command) ?
		       native_mobile_birth_cash_role_recovery_decode(command, attachment, output) :
		       native_mobile_birth_recovery_decode(command, attachment, output);
}
bool birth_recovery_valid(const critical_native_recovery_envelope &envelope) noexcept
{
	if (shared_shop_command(envelope.command))
		return native_mobile_birth_shared_shop_recovery_valid(envelope);
	return ordinary_wallet_command(envelope.command) ?
		       native_mobile_birth_cash_role_recovery_valid(envelope) :
		       native_mobile_birth_recovery_valid(envelope);
}
bool birth_recovery_initial(const critical_native_recovery_envelope &envelope) noexcept
{
	if (shared_shop_command(envelope.command))
		return native_mobile_birth_shared_shop_recovery_initial(envelope);
	return ordinary_wallet_command(envelope.command) ?
		       native_mobile_birth_cash_role_recovery_initial(envelope) :
		       native_mobile_birth_recovery_initial(envelope);
}
bool birth_recovery_terminal(const critical_native_recovery_envelope &envelope) noexcept
{
	if (shared_shop_command(envelope.command))
		return native_mobile_birth_shared_shop_recovery_terminal(envelope);
	return ordinary_wallet_command(envelope.command) ?
		       native_mobile_birth_cash_role_recovery_terminal(envelope) :
		       native_mobile_birth_recovery_terminal(envelope);
}
bool birth_recovery_successor(const critical_native_recovery_envelope &expected,
			      const critical_native_recovery_envelope &successor) noexcept
{
	if (shared_shop_command(expected.command))
		return native_mobile_birth_shared_shop_recovery_successor(expected, successor);
	return ordinary_wallet_command(expected.command) ?
		       native_mobile_birth_cash_role_recovery_successor(expected, successor) :
		       native_mobile_birth_recovery_successor(expected, successor);
}
#ifndef __NO_MYSQL__
unsigned int lock_birth_publication(MYSQL *connection, const critical_command &command,
				    const critical_completion &completion,
				    quest_mobile_native_image *image,
				    std::vector<item_ownership_runtime_entry> *custody,
				    const flatfile_shopkeeper_record *checkpoint = nullptr) noexcept
{
	if (shared_shop_command(command))
		return checkpoint ? economic_sql_native_mobile_birth_shared_shop_lock_publication(
					    connection, command, *checkpoint, completion, image,
					    custody) :
				    EILSEQ;
	return ordinary_wallet_command(command) ?
		       economic_sql_native_mobile_birth_ordinary_wallet_lock_publication(
			       connection, command, completion, image, custody) :
		       economic_sql_native_mobile_birth_lock_publication(
			       connection, command, completion, image, custody);
}
// These values are read only after the original SQL/source cut. The actual
// wallet mapping is carried by the authenticated receipt, never the native UID.
struct birth_wallet_receipt_values
{
	uint64_t mobile_instance_id = 0, cash_revision = 0, wallet_mapping_id = 0;
};
// A shared MBR4 has an intentionally zero ordinary-wallet mapping. Decode
// and bind its complete original participant/plan without weakening the
// ordinary wallet observer or treating zero as an invented wallet identity.
bool decode_shared_birth_receipt(const critical_command &command,
				 const critical_completion &completion,
				 birth_wallet_receipt_values *output) noexcept
{
	if (!output || !shared_shop_command(command) ||
	    completion.operation_id.bytes != command.operation_id.bytes ||
	    completion.disposition != critical_completion_disposition::execution ||
	    (completion.outcome != critical_apply_outcome::applied &&
	     completion.outcome != critical_apply_outcome::already_applied) ||
	    completion.error_code || completion.failure_stage != critical_failure_stage::none ||
	    completion.result_size > completion.result_payload.size())
		return false;
	try
	{
		native_mobile_birth_cash_role_result result;
		economic_frozen_intent intent;
		economic_accounting_plan plan;
		if (!native_mobile_birth_cash_role_result_decode({ completion.result_payload.data(),
								   completion.result_size },
								 &result) ||
		    result.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    result.wallet_mapping_id ||
		    economic_intent_decode(command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(command, intent) !=
			    economic_accounting_error::ok ||
		    native_mobile_birth_cash_role_accounting_compile(
			    command, result.shared, &plan) != economic_accounting_error::ok ||
		    !native_mobile_birth_cash_role_result_matches(command, result.shared, plan,
								  result) ||
		    completion.durable_revision !=
			    std::max<uint64_t>(1, result.shared.owner_revision_after))
			return false;
		*output = { result.mobile_instance_id, result.cash_revision, 0 };
		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool decode_birth_wallet_receipt(const critical_command &command,
				 const critical_completion &completion,
				 birth_wallet_receipt_values *output) noexcept
{
	if (shared_shop_command(command))
		return decode_shared_birth_receipt(command, completion, output);
	if (!ordinary_wallet_command(command))
	{
		native_mobile_birth_result result;
		if (!native_mobile_birth_result_decode(
			    { completion.result_payload.data(), completion.result_size }, &result))
			return false;
		*output = { result.mobile_instance_id, result.cash_revision,
			    result.wallet_mapping_id };
		return true;
	}
	try
	{
		native_mobile_birth_cash_role_result result;
		economic_frozen_intent intent;
		if (!native_mobile_birth_cash_role_result_decode({ completion.result_payload.data(),
								   completion.result_size },
								 &result) ||
		    result.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    !result.wallet_mapping_id ||
		    economic_intent_decode(command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(command, intent) !=
			    economic_accounting_error::ok)
			return false;
		const economic_account_key wallet{ intent.admission.metadata.lineage,
						   economic_account_kind::wallet,
						   result.wallet_mapping_id,
						   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		economic_accounting_plan plan;
		if (native_mobile_birth_cash_role_accounting_compile(command, wallet, &plan) !=
			    economic_accounting_error::ok ||
		    !native_mobile_birth_cash_role_result_matches(command, wallet, plan, result))
			return false;
		*output = { result.mobile_instance_id, result.cash_revision,
			    result.wallet_mapping_id };
		return true;
	}
	catch (...)
	{
		return false;
	}
}
#endif

#ifndef __NO_MYSQL__
bool retain_published_constructor_origin(const critical_native_recovery_envelope &original,
					 void *) noexcept
{
	try
	{
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool confirmed = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17) ||
			    !transaction.same_session() ||
			    !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
			    (shared_shop_command(original.command) ?
				     quest_mobile_native_origin_sql_retain_shared_shop_locked(
					     connection, original) :
			     ordinary_wallet_command(original.command) ?
				     quest_mobile_native_origin_sql_retain_ordinary_wallet_locked(
					     connection, original) :
				     quest_mobile_native_origin_sql_retain_locked(connection,
										  original)) ||
			    !transaction.same_session() ||
			    !(connection->server_status & SERVER_STATUS_IN_TRANS))
				throw EAGAIN;
			transaction.committing();
			if (mysql_real_query(connection, "COMMIT", 6) || !transaction.committed())
				throw EAGAIN;
			confirmed = true;
		}
		catch (...)
		{
			confirmed = false;
		}
		transaction.finish();
		// Uncertain commits retire their lease even when a later ROLLBACK can
		// confirm an idle session. Only the exact retained origin can reconcile
		// that attempt on the next original, still-fenced callback.
		if (confirmed || !transaction.commit_attempted())
			lease.reuse(cleanup);
		return confirmed && transaction.same_session() &&
		       cleanup.disposition == player_sql_cleanup_disposition::idle_verified &&
		       !cleanup.cleanup_error;
	}
	catch (...)
	{
		return false;
	}
}
#endif
struct original_item
{
	std::unique_ptr<quest_mobile_native_item_stage> stage;
	P_obj object = nullptr;
	uint64_t uid = 0;
	int rnum = -1;
	bool adopted_metadata = false;
	bool published = false, enrolled = false, enrollment_started = false,
	     enrollment_returned = false;
	std::vector<quest_mobile_native_item_effect> effects;
};
// Exact bounded writers are allocated/encoded BEFORE original effects or CAS.
struct original_birth_checkpoint
{
	critical_native_recovery_envelope expected, successor;
	native_mobile_birth_recovery_context context;
	bool consumes_choice = false;
};
struct original_birth
{
	quest_mobile_native_stage mobile;
	P_char character = nullptr;
	uint64_t runtime_id = 0;
	int zone = -1, room = NOWHERE, rnum = -1, shop = -1;
	quest_mobile_native_reference reference;
	std::vector<original_item> stock;
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	quest_mobile_native_constructor_recipe constructor;
	bool constructor_present = false;
	native_mobile_birth_cash_role_recipe cash_role;
	bool cash_role_present = false;
	bool non_alchemist_cut_returned = false;
	bool alchemist_started = false, alchemist_returned = false;
	critical_command command{};
	std::vector<uint8_t> canonical;
	// Installed once from the genuine final detached cut, or from the complete
	// passive original envelope. No progress writer may replace these bytes.
	std::unique_ptr<const std::vector<uint8_t>> shared_checkpoint;
	int64_t shared_saved_at = -1;
	bool shared_source_cut = false;
	std::vector<item_ownership_runtime_entry> current_custody;
	critical_completion completion{};
	critical_native_recovery_envelope envelope;
	native_mobile_birth_recovery_context recovery;
	std::unique_ptr<original_birth_checkpoint> checkpoint;
	std::unique_ptr<critical_native_recovery_envelope> ack_successor;
	std::array<std::unique_ptr<original_birth_checkpoint>, 4> returned_writers;
	uint8_t pending_action = 0;
	size_t pending_row = 0, pending_step = 0;
	bool action_not_attempted = false;
	// Choice remains in RAM if encoding/initial CAS refuses; never reroll it.
	std::unique_ptr<native_mobile_birth_recovery_context> chosen_context;
	shop_trade_original_procedure_binding_stage bindings;
	uint64_t coordinator_generation = 0;
	uint64_t accepted_usec = 0;
	size_t mobile_bytes = 0;
	bool sealed = false, submitted = false, cold = false, blocked = false;
	bool cold_adopted = false, cold_rebuilding = false;
	// Only this restored private stage owns saved-affect service latches.
	bool shared_cold_affects = false;
	// Passive original replay enrollment survives cold reconstruction until retire.
	// This RAM-only correlation never grants SQL or coordinator authority.
	bool cold_replay_enrolled = false;
	bool completed = false, binding_committed = false, mobile_started = false;
	bool mobile_consumed = false, runtime_applied = false, physically_proven = false;
	bool retired = false;
	bool refusal_cleanup_started = false, refusal_cleanup_returned = false;
};
bool ordinary_cash_role_current(const original_birth &birth) noexcept
{
	if (!birth.cash_role_present ||
	    birth.cash_role.role != native_mobile_birth_cash_role::ordinary_wallet)
		return false;
	native_mobile_birth_cash_role_recipe observed;
	native_mobile_birth_cash_role_recipe_bytes expected{}, actual{};
	return native_mobile_birth_cash_role_recipe_capture(birth.constructor, &observed) &&
	       native_mobile_birth_cash_role_recipe_encode(birth.cash_role, &expected) &&
	       native_mobile_birth_cash_role_recipe_encode(observed, &actual) && expected == actual;
}
bool shared_cash_role_current(const original_birth &birth) noexcept
{
	if (!birth.cash_role_present ||
	    birth.cash_role.role != native_mobile_birth_cash_role::shared_shopkeeper)
		return false;
	native_mobile_birth_cash_role_recipe observed;
	native_mobile_birth_cash_role_recipe_bytes expected{}, actual{};
	return native_mobile_birth_cash_role_recipe_capture(birth.constructor, &observed) &&
	       native_mobile_birth_cash_role_recipe_encode(birth.cash_role, &expected) &&
	       native_mobile_birth_cash_role_recipe_encode(observed, &actual) && expected == actual;
}
// Value-only role observer; no source or admission capability follows.
bool shared_birth(const original_birth &birth) noexcept
{
	return (birth.cash_role_present &&
		birth.cash_role.role == native_mobile_birth_cash_role::shared_shopkeeper) ||
	       shared_shop_command(birth.command);
}
// Complete current saved keeper values with the strong native reciprocal
// literal observer. Before original room step1 returns, NOWHERE is observed
// honestly and the authenticated original destination remains pending data.
// No room service, scheduler action or temporary in_room assignment occurs.
bool shared_keeper_checkpoint_current(const original_birth &birth, P_char actual,
				      uint64_t observed_runtime) noexcept
{
	if (!actual || !observed_runtime || actual->runtime_id != observed_runtime ||
	    !IS_NPC(actual) || !actual->only.npc || !IS_ALIVE(actual) || !birth.shared_checkpoint ||
	    !shop_index || number_of_shops < 0 || birth.shop < 0 || birth.shop >= number_of_shops ||
	    !world || !mob_index || birth.room < 0 || birth.room > top_of_world ||
	    (birth.recovery.mobile_effects[1].succeeded ? actual->in_room != birth.room :
							  actual->in_room != NOWHERE))
		return false;
	try
	{
		flatfile_shopkeeper_record original, observed;
		if (!flatfile_shopkeeper_initial_checkpoint_decode(*birth.shared_checkpoint,
								   &original))
			return false;
		const int rnum = GET_RNUM(actual);
		if (rnum < 0 || rnum > top_of_mobt || !IS_SHOPKEEPER(actual) ||
		    actual->only.npc->shopkeeper_shop_id != birth.shop ||
		    shop_index[birth.shop].keeper != rnum ||
		    GET_BIRTHPLACE(actual) != birth.reference.birthplace_vnum ||
		    original.shop_id != static_cast<uint32_t>(birth.shop) ||
		    original.room_vnum != world[birth.room].number || GET_COPPER(actual) < 0 ||
		    GET_SILVER(actual) < 0 || GET_GOLD(actual) < 0 || GET_PLATINUM(actual) < 0)
			return false;
		observed.shop_id = static_cast<uint32_t>(birth.shop);
		observed.mob_vnum = mob_index[rnum].virtual_number;
		observed.room_vnum = world[birth.room].number;
		observed.revision = 1;
		observed.saved_at =
			original.saved_at; // Original stamp, independently proven by SQL.
		observed.roaming = shop_index[birth.shop].shop_is_roaming != 0;
		observed.cash = static_cast<int64_t>(GET_COPPER(actual)) +
				10LL * GET_SILVER(actual) + 100LL * GET_GOLD(actual) +
				1000LL * GET_PLATINUM(actual);
		if (observed.cash > INT_MAX)
			return false;
		// A finite bounded walk proves the actual complete list, including
		// NOSAVE nodes omitted by the existing keeper serialization policy.
		const affected_type *slow = actual->affected, *fast = actual->affected;
		while (fast && fast->next)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
				return false;
		}
		size_t walked = 0;
		for (const affected_type *affect = actual->affected; affect; affect = affect->next)
		{
			if (++walked > 4096)
				return false;
			if (IS_SET(affect->flags, AFFTYPE_NOSAVE))
				continue;
			observed.affects.push_back(
				{ affect->type,
				  affect->duration,
				  affect->modifier,
				  affect->location,
				  { affect->bitvector, affect->bitvector2, affect->bitvector3,
				    affect->bitvector4, affect->bitvector5 } });
		}
		std::vector<uint8_t> exact;
		return quest_mobile_native_items_observe(actual, birth.reference,
							 &observed.items) ==
			       player_snapshot_capture_result::ok &&
		       flatfile_shopkeeper_initial_checkpoint_encode(observed, &exact) &&
		       exact == *birth.shared_checkpoint;
	}
	catch (...)
	{
		return false;
	}
}
struct original_reset_request
{
	int zone, force;
};
std::vector<std::unique_ptr<original_birth>> births;
std::vector<original_reset_request> deferred;
critical_operation_id reset_invocation{};
int reset_zone_rnum = -1;
size_t current_birth = SIZE_MAX;
bool replay_ready = false, reset_in_progress = false, deferred_overflow = false;
struct original_reset_dispatch_cursor
{
	const reset_com *commands = nullptr;
	int32_t zone_vnum = -1;
	int force = 0, last_cmd = 1;
	uint64_t next_slot = 0;
	uint32_t current_slot = 0;
	char current_command = 0;
	bool valid = false, open = false, completed = false, aborted = false,
	     invocation_attempted = false;
	reset_com original_command{};
	quest_mobile_original_reset_locals locals;
	struct actor_hold
	{
		P_char character = nullptr;
		uint64_t runtime_id = 0;
		original_birth *owner = nullptr;
		critical_operation_id birth_operation{};
	} actors[4];
	bool held = false, retryable = false, resume_dispatch = false, entry_pending = false;
};
original_reset_dispatch_cursor reset_dispatch;
bool reset_dispatch_identity() noexcept
{
	return reset_in_progress && replay_ready && nevent_is_game_thread() &&
	       economic_gameplay_authority::active_regular_sql() && zone_table &&
	       reset_zone_rnum >= 0 && reset_zone_rnum <= top_of_zone_table &&
	       reset_dispatch.commands &&
	       zone_table[reset_zone_rnum].cmd == reset_dispatch.commands &&
	       zone_table[reset_zone_rnum].number == reset_dispatch.zone_vnum;
}
bool reset_dispatch_current() noexcept
{
	return reset_dispatch.valid && reset_dispatch_identity();
}
bool original_command_current() noexcept
{
	if (!reset_dispatch_current() || !reset_dispatch.open)
		return false;
	const auto &a = reset_dispatch.commands[reset_dispatch.current_slot];
	const auto &b = reset_dispatch.original_command;
	return a.command == b.command && a.if_flag == b.if_flag && a.arg1 == b.arg1 &&
	       a.arg2 == b.arg2 && a.arg3 == b.arg3 && a.arg4 == b.arg4;
}
bool actor_hold_current(const original_reset_dispatch_cursor::actor_hold &hold) noexcept
{
	if (!hold.character)
		return !hold.runtime_id && !hold.owner;
	if (!hold.runtime_id)
		return false;
	if (!hold.owner)
		return find_character_by_runtime_id(hold.runtime_id) == hold.character;
	for (const auto &candidate : births)
		if (candidate && candidate.get() == hold.owner)
			return candidate->character == hold.character &&
			       candidate->runtime_id == hold.runtime_id &&
			       candidate->reference.birth_operation.bytes ==
				       hold.birth_operation.bytes &&
			       !candidate->mobile_consumed &&
			       candidate->mobile.character() == hold.character;
	return false;
}
bool capture_actor_hold(P_char actor, original_reset_dispatch_cursor::actor_hold *output) noexcept
{
	if (!output)
		return false;
	if (!actor)
	{
		*output = {};
		return true;
	}
	for (const auto &b : births)
		if (b && b->character == actor && !b->mobile_consumed &&
		    b->mobile.character() == actor && b->runtime_id)
		{
			*output = { actor, b->runtime_id, b.get(), b->reference.birth_operation };
			return true;
		}
	// The original live list supplies a genuine current pointer, then the
	// runtime index seals its generation. Never find a replacement by VNUM.
	if (!character_runtime_index_is_consistent())
		return false;
	for (P_char live = character_list; live; live = live->next)
		if (live == actor && live->runtime_id &&
		    find_character_by_runtime_id(live->runtime_id) == live)
		{
			*output = { actor, live->runtime_id, nullptr, {} };
			return true;
		}
	return false;
}
bool reset_actor_holds_current() noexcept
{
	const auto &locals = reset_dispatch.locals;
	const P_char actors[] = { locals.mob, locals.last_mob, locals.tmp_mob,
				  locals.last_mob_followable };
	for (size_t i = 0; i < 4; ++i)
		if (reset_dispatch.actors[i].character != actors[i] ||
		    !actor_hold_current(reset_dispatch.actors[i]))
			return false;
	return true;
}
bool reset_holds_birth(const original_birth &birth) noexcept
{
	if (!reset_in_progress || reset_dispatch.completed)
		return false;
	if (!critical_operation_id_is_zero(reset_invocation) &&
	    birth.reference.birth_source.source.bytes == reset_invocation.bytes &&
	    birth.reference.birth_source.generation.bytes == reset_invocation.bytes &&
	    birth.zone == reset_zone_rnum && !birth.mobile_consumed)
		return true;
	// The genuine original dispatcher owns its current unpublished body even
	// before the first yield; held aliases add their actual factory lifetimes.
	if (current_birth < births.size() && births[current_birth].get() == &birth)
		return true;
	for (const auto &hold : reset_dispatch.actors)
		if (hold.owner == &birth)
			return true;
	return false;
}
bool reset_resume_current() noexcept
{
	if (!reset_dispatch.held || !reset_dispatch.retryable || !reset_dispatch_current() ||
	    reset_dispatch.aborted || reset_dispatch.completed)
		return false;
	if (reset_dispatch.entry_pending)
		return !reset_dispatch.open && reset_dispatch.next_slot == 0 &&
		       !reset_dispatch.locals.initialized &&
		       critical_operation_id_is_zero(reset_invocation);
	return original_command_current() && reset_actor_holds_current();
}
bool reset_invocation_ready() noexcept
{
	if (!critical_operation_id_is_zero(reset_invocation))
		return true;
	if (!reset_dispatch_identity() || reset_dispatch.invocation_attempted)
		return false;
	reset_dispatch.invocation_attempted = true;
	critical_operation_id invocation{};
	if (!critical_operation_id_generate(&invocation))
		return false;
	reset_invocation = invocation;
	return true;
}
bool reset_dispatch_source(uint32_t slot, char command, int room, bool current,
			   economic_source_event *source, int32_t *zone_vnum) noexcept
{
	if (!source || !zone_vnum || !reset_dispatch_current())
		return false;
	if (current ? (!reset_dispatch.open || reset_dispatch.completed ||
		       reset_dispatch.current_slot != slot ||
		       reset_dispatch.current_command != command) :
		      !(slot < reset_dispatch.next_slot ||
			(reset_dispatch.open && reset_dispatch.current_slot == slot)))
		return false;
	const auto &original = reset_dispatch.commands[slot];
	if (original.command != command ||
	    (command == 'O' && (!world || room < 0 || room > top_of_world ||
				world[room].number <= 0 || original.arg3 != room)))
		return false;
	if (!reset_invocation_ready())
		return false;
	*source = { economic_source_kind::world_generation, reset_invocation, reset_invocation, 0,
		    slot };
	*zone_vnum = reset_dispatch.zone_vnum;
	return true;
}

bool alchemist_choice_compatible(P_char actor,
				 const quest_mobile_native_constructor_recipe &recipe) noexcept
{
	if (!actor || !IS_NPC(actor) || !actor->only.npc)
		return false;
	if (recipe.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION)
		return !GET_CLASS(actor, CLASS_ALCHEMIST);
	if (recipe.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
		return false;
	const bool eligible = GET_CLASS(actor, CLASS_ALCHEMIST) &&
			      !actor->only.npc->summoned_instance && !GET_MASTER(actor);
	switch (recipe.alchemist_choice)
	{
	case original_alchemist_choice::not_attempted:
		return !eligible;
	case original_alchemist_choice::missed:
	case original_alchemist_choice::selected:
		return eligible;
	default:
		return false;
	}
}
uint64_t accepted_now() noexcept
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::system_clock::now().time_since_epoch())
					     .count());
}
bool add_bytes(size_t &total, size_t bytes) noexcept
{
	if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - total)
		return false;
	total += bytes;
	return true;
}

bool charge_command(size_t &bytes, const critical_command &command) noexcept
{
	return add_bytes(bytes, command.payload.capacity()) &&
	       add_bytes(bytes, command.accounting_intent.capacity()) &&
	       add_bytes(bytes, command.keys.capacity() * sizeof(critical_entity_key)) &&
	       add_bytes(bytes, command.expected_revisions.capacity() *
					sizeof(critical_expected_revision));
}
bool charge_envelope(size_t &bytes, const critical_native_recovery_envelope &envelope) noexcept
{
	return charge_command(bytes, envelope.command) &&
	       add_bytes(bytes, envelope.attachment.capacity());
}
bool charge_context(size_t &bytes, const native_mobile_birth_recovery_context &context) noexcept
{
	if (!add_bytes(bytes, context.items.capacity() * sizeof(native_mobile_birth_recovery_item)))
		return false;
	for (const auto &item : context.items)
		if (!add_bytes(bytes, item.effects.capacity() *
					      sizeof(native_mobile_birth_recovery_effect)))
			return false;
	return true;
}
// Fixed original birth action domains, private to this translation unit.
enum : uint8_t
{
	BINDINGS = 1,
	REFERENCE = 2,
	ITEM_PUBLICATION = 3,
	MOBILE_EFFECT = 4,
	ITEM_ENROLLMENT = 5,
	ITEM_EFFECT = 6
};
native_mobile_birth_recovery_effect
action_value(const native_mobile_birth_recovery_context &context, uint8_t kind, size_t row,
	     size_t step)
{
	if (kind == MOBILE_EFFECT)
		return context.mobile_effects.at(step);
	if (kind == ITEM_EFFECT)
		return context.items.at(row).effects.at(step);
	const native_mobile_birth_recovery_action *action = nullptr;
	if (kind == BINDINGS)
		action = &context.whole_binding;
	else if (kind == REFERENCE)
		action = &context.reference_install;
	else if (kind == ITEM_PUBLICATION)
		action = &context.items.at(row).publication;
	else if (kind == ITEM_ENROLLMENT)
		action = &context.items.at(row).enrollment;
	else
		throw EILSEQ;
	return { action->started, action->returned, action->succeeded, false };
}
void set_action(native_mobile_birth_recovery_context &context, uint8_t kind, size_t row,
		size_t step, const native_mobile_birth_recovery_effect &effect)
{
	if (kind == MOBILE_EFFECT)
	{
		context.mobile_effects.at(step) = effect;
		if (step == 0)
		{
			context.mobile_publication.started = effect.started;
			context.mobile_publication.consumed = effect.returned && effect.succeeded;
		}
		if (step == 7)
			context.mobile_publication.returned = effect.returned && effect.succeeded;
		return;
	}
	if (kind == ITEM_EFFECT)
	{
		auto &item = context.items.at(row);
		item.effects.at(step) = effect;
		item.current_step_started = effect.started &&
					    !(effect.returned && effect.succeeded);
		if (effect.returned && effect.succeeded)
			item.next_step = static_cast<uint32_t>(step + 1);
		return;
	}
	native_mobile_birth_recovery_action *action = nullptr;
	if (kind == BINDINGS)
		action = &context.whole_binding;
	else if (kind == REFERENCE)
		action = &context.reference_install;
	else if (kind == ITEM_PUBLICATION)
	{
		auto &item = context.items.at(row);
		action = &item.publication;
		item.admitted = true;
		item.published = effect.returned && effect.succeeded;
	}
	else if (kind == ITEM_ENROLLMENT)
		action = &context.items.at(row).enrollment;
	else
		throw EILSEQ;
	*action = { effect.started, effect.returned, effect.succeeded };
}
bool same_completion(const critical_completion &a, const critical_completion &b) noexcept
{
	return a.operation_id.bytes == b.operation_id.bytes && a.outcome == b.outcome &&
	       a.durable_revision == b.durable_revision && a.error_code == b.error_code &&
	       a.failure_stage == b.failure_stage && a.disposition == b.disposition &&
	       a.result_size == b.result_size && a.result_payload == b.result_payload &&
	       a.attempt == b.attempt && a.queued_at_usec == b.queued_at_usec &&
	       a.started_at_usec == b.started_at_usec &&
	       a.completed_at_usec == b.completed_at_usec &&
	       a.recovery_correlation == b.recovery_correlation;
}

bool same_economic_completion(const critical_completion &a, const critical_completion &b) noexcept
{
	const auto success = [](const critical_completion &c)
	{
		return c.disposition == critical_completion_disposition::execution &&
		       (c.outcome == critical_apply_outcome::applied ||
			c.outcome == critical_apply_outcome::already_applied) &&
		       !c.error_code && c.failure_stage == critical_failure_stage::none;
	};
	return success(a) && success(b) && a.operation_id.bytes == b.operation_id.bytes &&
	       a.durable_revision == b.durable_revision && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload;
}
bool same_image(const quest_mobile_native_image &a, const quest_mobile_native_image &b)
{
	std::vector<uint8_t> left, right;
	return quest_mobile_native_image_encode(a, &left) == player_snapshot_codec_result::ok &&
	       quest_mobile_native_image_encode(b, &right) == player_snapshot_codec_result::ok &&
	       left == right;
}
[[maybe_unused]] bool actual_objects(const original_birth &b) noexcept
{
	for (const auto &item : b.stock)
		if (item.published)
		{
			P_obj found = nullptr;
			for (P_obj o = object_list; o; o = o->next)
				if (o->obj_uid == item.uid)
				{
					if (found || o != item.object || o->R_num != item.rnum)
						return false;
					found = o;
				}
			if (!found)
				return false;
		}
	return true;
}
}

namespace
{

#ifndef __NO_MYSQL__
bool validate_ordinary_progressed_origin(const critical_native_recovery_envelope &original,
					 const quest_mobile_native_image &current,
					 const native_mobile_wallet_origin &origin) noexcept
{
	try
	{
		if (!native_mobile_birth_cash_role_recovery_terminal(original) ||
		    current.state != quest_mobile_lifetime_state::live || !current.cash)
			return false;
		quest_mobile_native_image born;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		native_mobile_birth_recovery_context progress;
		economic_frozen_intent intent;
		native_mobile_birth_cash_role_result receipt;
		std::vector<uint8_t> canonical_current;
		if (native_mobile_birth_cash_role_command_decode(original.command, &born, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    native_mobile_birth_cash_role_recovery_decode(original.command,
								  original.attachment, &progress) !=
			    economic_accounting_error::ok ||
		    !progress.receipt_present || !born.cash ||
		    !native_mobile_birth_cash_role_result_decode(
			    { progress.receipt.result_payload.data(),
			      progress.receipt.result_size },
			    &receipt) ||
		    economic_intent_decode(original.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(original.command, intent) !=
			    economic_accounting_error::ok ||
		    quest_mobile_native_image_encode(current, &canonical_current) !=
			    player_snapshot_codec_result::ok)
			return false;
		auto lifetime = current.reference;
		if (lifetime.mobile_revision < born.reference.mobile_revision ||
		    lifetime.stock_revision < born.reference.stock_revision ||
		    current.cash->revision < born.cash->revision)
			return false;
		lifetime.mobile_revision = born.reference.mobile_revision;
		lifetime.stock_revision = born.reference.stock_revision;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> left{}, right{};
		if (quest_mobile_native_reference_encode(lifetime, &left) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(born.reference, &right) !=
			    player_snapshot_codec_result::ok ||
		    left != right ||
		    origin.birth_operation.bytes != born.reference.birth_operation.bytes ||
		    origin.mobile_instance_id != born.reference.mobile_instance_id ||
		    !origin.wallet_mapping_id ||
		    origin.lineage.bytes != intent.admission.metadata.lineage.bytes ||
		    origin.birth_epoch.bytes != intent.admission.metadata.epoch.bytes ||
		    receipt.wallet_mapping_id != origin.wallet_mapping_id)
			return false;
		const economic_account_key wallet{ origin.lineage, economic_account_kind::wallet,
						   origin.wallet_mapping_id,
						   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		economic_accounting_plan plan;
		return native_mobile_birth_cash_role_accounting_compile(
			       original.command, wallet, &plan) == economic_accounting_error::ok &&
		       native_mobile_birth_cash_role_result_matches(original.command, wallet, plan,
								    receipt);
	}
	catch (...)
	{
		return false;
	}
}
#endif
}

bool quest_mobile_native_birth_owner::validate_progressed_origin(
	const critical_native_recovery_envelope &original_birth,
	const quest_mobile_native_image &current,
	const native_mobile_wallet_origin &origin) noexcept
{
#ifdef __NO_MYSQL__
	(void)original_birth;
	(void)current;
	(void)origin;
	return false;
#else
	if (ordinary_wallet_command(original_birth.command))
		return validate_ordinary_progressed_origin(original_birth, current, origin);
	try
	{
		// The original terminal attachment stays immutable. A current economic
		// image is not a substitute for the historical constructor or receipt.
		if (original_birth.command.payload_version !=
			    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION ||
		    !native_mobile_birth_recovery_terminal(original_birth) ||
		    current.state != quest_mobile_lifetime_state::live || !current.cash)
			return false;
		quest_mobile_native_image born;
		std::vector<native_mobile_birth_item_recipe> recipes;
		quest_mobile_native_constructor_recipe constructor;
		native_mobile_birth_recovery_context progress;
		economic_frozen_intent intent;
		native_mobile_birth_result receipt;
		std::vector<uint8_t> canonical_current;
		if (native_mobile_birth_command_decode(original_birth.command, &born, &recipes,
						       &constructor) !=
			    economic_accounting_error::ok ||
		    (constructor.wire_version !=
			     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
		     constructor.wire_version !=
			     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) ||
		    native_mobile_birth_recovery_decode(original_birth.command,
							original_birth.attachment, &progress) !=
			    economic_accounting_error::ok ||
		    !progress.receipt_present || !born.cash ||
		    !native_mobile_birth_result_decode({ progress.receipt.result_payload.data(),
							 progress.receipt.result_size },
						       &receipt) ||
		    economic_intent_decode(original_birth.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(original_birth.command, intent) !=
			    economic_accounting_error::ok ||
		    quest_mobile_native_image_encode(current, &canonical_current) !=
			    player_snapshot_codec_result::ok)
			return false;
		// Compare the complete original lifetime, excluding only its two mutable
		// revisions. Current stock may include genuinely acquired player items;
		// equality here neither invents nor authenticates their restore recipes.
		auto lifetime = current.reference;
		if (lifetime.mobile_revision < born.reference.mobile_revision ||
		    lifetime.stock_revision < born.reference.stock_revision ||
		    current.cash->revision < born.cash->revision)
			return false;
		lifetime.mobile_revision = born.reference.mobile_revision;
		lifetime.stock_revision = born.reference.stock_revision;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> left{}, right{};
		if (quest_mobile_native_reference_encode(lifetime, &left) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(born.reference, &right) !=
			    player_snapshot_codec_result::ok ||
		    left != right ||
		    origin.birth_operation.bytes != born.reference.birth_operation.bytes ||
		    origin.mobile_instance_id != born.reference.mobile_instance_id ||
		    !origin.wallet_mapping_id ||
		    origin.lineage.bytes != intent.admission.metadata.lineage.bytes ||
		    origin.birth_epoch.bytes != intent.admission.metadata.epoch.bytes ||
		    receipt.wallet_mapping_id != origin.wallet_mapping_id)
			return false;
		const economic_account_key wallet{ origin.lineage, economic_account_kind::wallet,
						   origin.wallet_mapping_id,
						   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		economic_accounting_plan original_plan;
		return native_mobile_birth_accounting_compile(original_birth.command, wallet,
							      &original_plan) ==
			       economic_accounting_error::ok &&
		       native_mobile_birth_result_matches(original_birth.command, wallet,
							  original_plan, receipt);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_mobile_native_birth_owner::charge() noexcept
{
	try
	{
		size_t bytes = 0;
		if (reset_in_progress && !add_bytes(bytes, sizeof(reset_dispatch)))
			return false;
		if (!add_bytes(bytes, births.capacity() * sizeof(births[0])) ||
		    !add_bytes(bytes, deferred.capacity() * sizeof(deferred[0])))
			return false;
		for (const auto &ptr : births)
			if (ptr)
			{
				const auto &b = *ptr;
				if (!add_bytes(bytes, sizeof(b)) ||
				    !add_bytes(bytes, b.stock.capacity() * sizeof(original_item)) ||
				    !add_bytes(bytes, b.canonical.capacity()) ||
				    (b.shared_checkpoint &&
				     (!add_bytes(bytes, sizeof(*b.shared_checkpoint)) ||
				      !add_bytes(bytes, b.shared_checkpoint->capacity()))) ||
				    !add_bytes(bytes, b.command.payload.capacity()) ||
				    !add_bytes(bytes, b.command.accounting_intent.capacity()) ||
				    !add_bytes(bytes, b.command.keys.capacity() *
							      sizeof(critical_entity_key)) ||
				    !add_bytes(bytes, b.command.expected_revisions.capacity() *
							      sizeof(critical_expected_revision)) ||
				    !add_bytes(bytes,
					       b.current_custody.capacity() *
						       sizeof(item_ownership_runtime_entry)) ||
				    !add_bytes(bytes, b.image.items.capacity() *
							      sizeof(player_item_snapshot)))
					return false;
				if (!add_bytes(bytes,
					       b.recipes.capacity() *
						       sizeof(native_mobile_birth_item_recipe)))
					return false;
				for (const auto &recipe : b.recipes)
					if (!add_bytes(
						    bytes,
						    recipe.libraries.capacity() *
							    sizeof(native_mobile_birth_library_recipe)))
						return false;
				if (!b.mobile.shared_shopkeeper_affect_charge(&bytes) ||
				    !charge_envelope(bytes, b.envelope) ||
				    !charge_context(bytes, b.recovery))
					return false;
				if (b.ack_successor &&
				    (!add_bytes(bytes, sizeof(*b.ack_successor)) ||
				     !charge_envelope(bytes, *b.ack_successor)))
					return false;
				const auto charge_writer =
					[&](const original_birth_checkpoint *writer)
				{
					return !writer ||
					       (add_bytes(bytes, sizeof(*writer)) &&
						charge_envelope(bytes, writer->expected) &&
						charge_envelope(bytes, writer->successor) &&
						charge_context(bytes, writer->context));
				};
				if (!charge_writer(b.checkpoint.get()))
					return false;
				for (const auto &writer : b.returned_writers)
					if (!charge_writer(writer.get()))
						return false;
				if (b.chosen_context &&
				    (!add_bytes(bytes, sizeof(*b.chosen_context)) ||
				     !charge_context(bytes, *b.chosen_context)))
					return false;
				const size_t binding_bytes = b.bindings.retained_bytes();
				if (!binding_bytes || binding_bytes < sizeof(b.bindings) ||
				    !add_bytes(bytes, binding_bytes - sizeof(b.bindings)))
					return false;
				for (const auto &row : b.image.items)
				{
					if (!add_bytes(bytes, row.name.capacity() + 1) ||
					    !add_bytes(bytes,
						       row.short_description.capacity() + 1) ||
					    !add_bytes(bytes, row.description.capacity() + 1) ||
					    !add_bytes(bytes,
						       row.action_description.capacity() + 1) ||
					    !add_bytes(
						    bytes,
						    row.dynamic_affects.capacity() *
							    sizeof(player_item_dynamic_affect_snapshot)) ||
					    !add_bytes(
						    bytes,
						    row.extra_descriptions.capacity() *
							    sizeof(player_item_extra_description_snapshot)))
						return false;
					for (const auto &description : row.extra_descriptions)
						if (!add_bytes(bytes,
							       description.keyword.capacity() +
								       1) ||
						    !add_bytes(bytes,
							       description.description.capacity() +
								       1) ||
						    !add_bytes(bytes,
							       description.spell_ids.capacity() *
								       sizeof(int32_t)))
							return false;
				}
				if (!b.mobile_consumed && !add_bytes(bytes, b.mobile_bytes))
					return false;
				for (const auto &item : b.stock)
				{
					if (!add_bytes(
						    bytes,
						    item.effects.capacity() *
							    sizeof(quest_mobile_native_item_effect)))
						return false;
					if (item.stage)
					{
						const size_t retained =
							item.stage->retained_bytes();
						if (!retained || !add_bytes(bytes, retained))
							return false;
					}
				}
			}
		size_t warm_bytes = 0;
		if (!zone_reset_item_owner::warm_retained_size(&warm_bytes) ||
		    !add_bytes(bytes, warm_bytes))
			return false;
		return item_native_quest_birth_budget_owner::retained_budget(bytes);
	}
	catch (...)
	{
		return false;
	}
}
size_t quest_mobile_native_birth_owner::pending_mobiles(int rnum) noexcept
{
	size_t count = 0;
	for (const auto &b : births)
		if (b && !b->retired && !b->mobile_consumed && b->rnum == rnum)
			++count;
	return count;
}
size_t quest_mobile_native_birth_owner::pending_items(int rnum) noexcept
{
	size_t count = 0;
	const bool accounted = economic_gameplay_authority::active();
	// Actual quota callers add an int obj_index.number in int64_t, then
	// compare against an int reset limit. Even INT_MIN + UINT_MAX reaches
	// INT_MAX, so this representable refusal cannot wrap into admission.
	constexpr size_t refused = static_cast<size_t>(UINT_MAX);
	for (const auto &b : births)
		if (b && !b->retired)
			for (const auto &item : b->stock)
				if (!item.published && item.rnum == rnum)
				{
					if (accounted && count == refused)
						return refused;
					++count;
				}
	if (!accounted)
		return count; // Preserve original inactive counting behavior.
	size_t warm = 0;
	if (!zone_reset_item_owner::pending_items(rnum, &warm) || warm > refused ||
	    count > refused - warm)
		return refused;
	return count + warm;
}
bool quest_mobile_native_birth_owner::pending_shop(int rnum, int room, int shop) noexcept
{
	for (const auto &b : births)
		if (b && !b->retired && !b->mobile_consumed &&
		    ((b->rnum == rnum && (b->room == room || b->cold)) ||
		     (shop >= 0 && b->shop == shop && !is_replicated_shop(shop))))
			return true;
	return false;
}
bool quest_mobile_native_birth_owner::begin_reset(int zone, int force) noexcept
{
	if (!economic_gameplay_authority::active())
		return true;
	if (!nevent_is_game_thread() || (!economic_gameplay_authority::active_regular_sql() &&
					 !economic_gameplay_authority::active_sql_recovery()))
		return false;
	try
	{
		// Only the owning pulse may reenter the exact retained invocation.
		// Ordinary reset requests continue through the existing bounded queue.
		if (reset_dispatch.resume_dispatch)
		{
			if (zone != reset_zone_rnum || force != reset_dispatch.force ||
			    !reset_resume_current() || !reset_objects_current() || !charge())
				return false;
			return true;
		}
		// Retain genuine bounded/deduplicated reset requests while selected
		// recovery keeps accounting policy active and fresh admission closed.
		bool held = economic_gameplay_authority::active_sql_recovery() || !replay_ready ||
			    reset_in_progress;
		// Same original zone VNUM: retain the existing request while genuine
		// room O/P owners still hold an unresolved warm invocation. The passive
		// observer cannot prepare SQL/native work or fabricate a new reset.
		if (zone_table && zone >= 0 && zone <= top_of_zone_table &&
		    zone_reset_room_item_warm_pending_vnum(zone_table[zone].number))
			held = true;
		for (const auto &b : births)
			if (b && !b->retired && b->zone == zone)
				held = true;
		if (held)
		{
			const auto found =
				std::find_if(deferred.begin(), deferred.end(), [&](const auto &r)
					     { return r.zone == zone && r.force == force; });
			if (found == deferred.end())
			{
				if (deferred.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS)
				{
					deferred_overflow = true;
					return false;
				}
				deferred.push_back({ zone, force });
				if (!charge())
				{
					deferred_overflow = true;
					return false;
				}
			}
			return false;
		}
		if (!zone_table || zone < 0 || zone > top_of_zone_table || !zone_table[zone].cmd)
			return false;
		reset_dispatch = {};
		reset_dispatch.commands = zone_table[zone].cmd;
		reset_dispatch.zone_vnum = zone_table[zone].number;
		reset_dispatch.force = force;
		reset_dispatch.valid = true;
		reset_in_progress = true;
		reset_zone_rnum = zone;
		reset_invocation = {};
		current_birth = SIZE_MAX;
		if (!charge())
		{
			reset_dispatch.held = true;
			reset_dispatch.retryable = true;
			reset_dispatch.entry_pending = true;
			return false;
		}
		return true;
	}
	catch (...)
	{
		deferred_overflow = true;
		return false;
	}
}
quest_mobile_original_reset_locals *
quest_mobile_native_birth_owner::original_reset_locals(int zone, int force) noexcept
{
	if (!reset_dispatch_current() || zone != reset_zone_rnum || force != reset_dispatch.force)
		return nullptr;
	if (reset_dispatch.resume_dispatch && (!reset_resume_current() || !reset_objects_current()))
		return nullptr;
	return &reset_dispatch.locals;
}
bool quest_mobile_native_birth_owner::reset_objects_current() noexcept
{
	const auto &locals = reset_dispatch.locals;
	const auto current = [](P_obj object, uint64_t uid, bool allow_warm = false) noexcept
	{
		if (!object)
			return uid == 0;
		if (!uid)
			return false;
		for (const auto &birth : births)
			if (birth)
				for (const auto &item : birth->stock)
					if (item.object == object && item.uid == uid &&
					    item.stage && !item.published &&
					    item.stage->object() == object)
						return true;
		// Only original O or room-P cursors borrow retained warm factory lifetimes.
		// This checks the remembered pointer/UID; it never chooses a replacement.
		if (allow_warm && zone_reset_item_owner::warm_object_current(object, uid))
			return true;
		// Equality is checked before reading the current world body. A recycled
		// address cannot pass the retained UID; no replacement body is selected.
		for (P_obj live = object_list; live; live = live->next)
			if (live == object)
				return live->obj_uid == uid;
		return false;
	};
	if (reset_dispatch.current_command == 'O' && locals.o.begun)
	{
		const auto &progress = locals.o;
		if (!progress.incumbent_returned &&
		    (progress.incumbent || progress.incumbent_uid || progress.incumbent_take))
			return false;
		if (progress.incumbent_returned &&
		    !current(progress.incumbent, progress.incumbent_uid, true))
			return false;
		return current(locals.obj, progress.object_uid, true) && current(locals.obj_to, 0);
	}
	if (reset_dispatch.current_command == 'P' && locals.p.room_path)
		return current(locals.obj, locals.p.object_uid, true) &&
		       current(locals.obj_to, locals.p.target_uid, true);
	return current(locals.obj, locals.p.object_uid) &&
	       current(locals.obj_to, locals.p.target_uid);
}
bool quest_mobile_native_birth_owner::hold_reset(uint32_t slot, int last_cmd,
						 bool retryable) noexcept
{
	// Preserve the real current slot BEFORE any census/refusing observation.
	reset_dispatch.held = true;
	reset_dispatch.retryable = false;
	if (!reset_dispatch_current() || !reset_dispatch.open || reset_dispatch.completed ||
	    reset_dispatch.aborted || reset_dispatch.current_slot != slot ||
	    reset_dispatch.locals.cmd_no != static_cast<int>(slot) || !original_command_current() ||
	    !reset_invocation_ready())
		return false;
	reset_dispatch.last_cmd = last_cmd;
	const auto &locals = reset_dispatch.locals;
	const P_char actors[] = { locals.mob, locals.last_mob, locals.tmp_mob,
				  locals.last_mob_followable };
	for (size_t i = 0; i < 4; ++i)
		if (!capture_actor_hold(actors[i], &reset_dispatch.actors[i]))
			return false;
	if (!reset_objects_current())
		return false;
	const auto &progress = locals.p;
	// Automatic continuation is restricted to the actual original P pure
	// target-observation cut. A caller flag cannot authorize retrying a factory,
	// load roll, discard, nesting leg or an uncertain returned failure.
	reset_dispatch.retryable =
		retryable && reset_dispatch.current_command == 'P' && locals.command_entered &&
		progress.begun && progress.eligible && progress.factory_started &&
		progress.factory_returned && locals.obj && progress.artifact_started &&
		progress.artifact_returned && !progress.artifact_owned &&
		!progress.target_returned && !progress.load_started && !progress.load_returned &&
		!progress.discard_started && !progress.nest_started;
	if (reset_dispatch.current_command == 'O')
	{
		const auto &original = locals.o;
		// Only the original warm owner's known pure cut may continue. Its actual
		// constructor/load/cleanup progress is not inferred from caller flags.
		reset_dispatch.retryable = retryable && locals.command_entered && original.begun &&
					   original.eligible && !original.root_returned &&
					   zone_reset_item_owner::warm_capture_retryable(slot);
	}
	else if (reset_dispatch.current_command == 'P' && progress.room_path)
	{
		// The actual room-P caller chose this route in the retained original
		// frame. Its warm owner alone proves the current known-pure phase.
		reset_dispatch.retryable = retryable && locals.command_entered && progress.begun &&
					   progress.eligible &&
					   zone_reset_item_owner::warm_capture_retryable(slot);
	}
	return charge();
}
void quest_mobile_native_birth_owner::observe_reset_command(uint32_t slot, int last_cmd) noexcept
{
	if (reset_dispatch.resume_dispatch && reset_dispatch.entry_pending)
	{
		// This exact front door held before its first command opened. There is
		// no prior RNG, constructor or invocation to repeat.
		reset_dispatch.resume_dispatch = false;
		reset_dispatch.entry_pending = false;
		reset_dispatch.held = false;
		reset_dispatch.retryable = false;
	}
	if (reset_dispatch.resume_dispatch)
	{
		reset_dispatch.resume_dispatch = false;
		if (!reset_dispatch.held || !reset_dispatch.retryable ||
		    reset_dispatch.current_slot != slot || reset_dispatch.last_cmd != last_cmd ||
		    !original_command_current() || !reset_actor_holds_current() ||
		    !reset_objects_current())
		{
			reset_dispatch.valid = false;
			return;
		}
		reset_dispatch.held = false;
		reset_dispatch.retryable = false;
		return; // This original slot was already opened; never reopen/redecide it.
	}
	if (!reset_dispatch_current() || reset_dispatch.open || reset_dispatch.completed ||
	    reset_dispatch.aborted || reset_dispatch.next_slot != slot ||
	    reset_dispatch.last_cmd != last_cmd)
	{
		reset_dispatch.valid = false;
		return;
	}
	reset_dispatch.current_slot = slot;
	reset_dispatch.current_command = reset_dispatch.commands[slot].command;
	reset_dispatch.original_command = reset_dispatch.commands[slot];
	reset_dispatch.open = true;
}
void quest_mobile_native_birth_owner::observe_reset_processed(uint32_t slot, int last_cmd) noexcept
{
	if (!reset_dispatch_current() || !reset_dispatch.open || reset_dispatch.completed ||
	    reset_dispatch.aborted || reset_dispatch.current_slot != slot ||
	    reset_dispatch.current_command == 'S')
	{
		reset_dispatch.valid = false;
		return;
	}
	reset_dispatch.last_cmd = last_cmd;
	reset_dispatch.next_slot = uint64_t(slot) + 1;
	reset_dispatch.open = false;
	reset_dispatch.locals.command_entered = false;
	reset_dispatch.locals.p = {};
	reset_dispatch.locals.o = {};
	reset_dispatch.locals.obj = reset_dispatch.locals.obj_to = nullptr;
	// Drop aliases only after the actual sequential command returned. The
	// current owner still holds its own body; no publication runs inside reset.
	for (auto &hold : reset_dispatch.actors)
		hold = {};
}
void quest_mobile_native_birth_owner::observe_reset_abort(uint32_t slot, int last_cmd) noexcept
{
	if (!reset_dispatch_current() || !reset_dispatch.open || reset_dispatch.completed ||
	    reset_dispatch.aborted || reset_dispatch.current_slot != slot)
	{
		reset_dispatch.valid = false;
		return;
	}
	reset_dispatch.last_cmd = last_cmd;
	reset_dispatch.open = false;
	reset_dispatch.aborted = true; // Retained refusal, never a processed slot or S.
}
void quest_mobile_native_birth_owner::observe_reset_stop(uint32_t slot, int last_cmd) noexcept
{
	if (!reset_dispatch_current() || !reset_dispatch.open || reset_dispatch.completed ||
	    reset_dispatch.current_slot != slot || reset_dispatch.current_command != 'S' ||
	    reset_dispatch.commands[slot].command != 'S' || reset_dispatch.last_cmd != last_cmd)
	{
		reset_dispatch.valid = false;
		return;
	}
	reset_dispatch.open = false;
	reset_dispatch.completed = true;
}
bool quest_mobile_native_birth_owner::capture_reset_dispatch_scope(economic_source_event *source,
								   int32_t *zone_vnum,
								   int *original_force) noexcept
{
	if (!source || !zone_vnum || !reset_dispatch_current() || reset_dispatch.aborted)
		return false;
	if (!reset_invocation_ready())
		return false;
	*source = { economic_source_kind::world_generation, reset_invocation, reset_invocation, 0,
		    reset_dispatch.current_slot };
	*zone_vnum = reset_dispatch.zone_vnum;
	if (original_force)
		*original_force = reset_dispatch.force;
	return true;
}
bool quest_mobile_native_birth_owner::capture_reset_abort_scope(economic_source_event *source,
								int32_t *zone_vnum,
								uint32_t *stop_slot,
								int *last_cmd) noexcept
{
	// Actual native finish owns this abort cut. No lazy generation/reissue or
	// completed cursor proof is manufactured if observation already failed.
	if (!source || !zone_vnum || !stop_slot || !last_cmd || !reset_dispatch_identity() ||
	    critical_operation_id_is_zero(reset_invocation))
		return false;
	*source = { economic_source_kind::world_generation, reset_invocation, reset_invocation, 0,
		    reset_dispatch.current_slot };
	*zone_vnum = reset_dispatch.zone_vnum;
	*stop_slot = reset_dispatch.current_slot;
	*last_cmd = reset_dispatch.last_cmd;
	return true;
}
bool quest_mobile_native_birth_owner::capture_reset_boundary(uint32_t slot, int last_cmd,
							     economic_source_event *source,
							     int32_t *zone_vnum) noexcept
{
	return reset_dispatch.completed && !reset_dispatch.open &&
	       reset_dispatch.current_slot == slot && reset_dispatch.last_cmd == last_cmd &&
	       capture_reset_dispatch_scope(source, zone_vnum);
}
bool quest_mobile_native_birth_owner::capture_room_reset_source(uint32_t slot, int room,
								economic_source_event *source,
								int32_t *zone_vnum) noexcept
{
	return reset_dispatch_source(slot, 'O', room, true, source, zone_vnum);
}
bool quest_mobile_native_birth_owner::capture_retained_room_reset_source(
	uint32_t slot, int room, economic_source_event *source, int32_t *zone_vnum) noexcept
{
	return reset_dispatch_source(slot, 'O', room, false, source, zone_vnum);
}
bool quest_mobile_native_birth_owner::capture_reset_child_source(uint32_t slot,
								 economic_source_event *source,
								 int32_t *zone_vnum) noexcept
{
	return reset_dispatch_source(slot, 'P', NOWHERE, true, source, zone_vnum);
}
bool quest_mobile_native_birth_owner::capture_retained_reset_child_source(
	uint32_t slot, economic_source_event *source, int32_t *zone_vnum) noexcept
{
	return reset_dispatch_source(slot, 'P', NOWHERE, false, source, zone_vnum);
}

P_char quest_mobile_native_birth_owner::prepare_mobile(int rnum, int room, uint32_t slot,
						       int shop) noexcept
{
	if (!reset_in_progress || !replay_ready || !nevent_is_game_thread() || room < 0 ||
	    room > top_of_world || rnum < 0 || rnum > top_of_mobt || !mob_index || !world)
		return nullptr;
	seal_mobile();
	try
	{
		size_t available = 0;
		while (available < births.size() && births[available])
			++available;
		if (available == births.size() &&
		    births.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS)
			return nullptr;
		auto b = std::make_unique<original_birth>();
		b->zone = reset_zone_rnum;
		b->room = room;
		b->rnum = rnum;
		b->shop = shop;
		if (available == births.size())
			births.reserve(births.size() + 1);
		if (!reset_invocation_ready())
			return nullptr;
		auto &ref = b->reference;
		if (!critical_operation_id_generate(&ref.birth_operation) ||
		    ref.birth_operation.bytes == reset_invocation.bytes)
			return nullptr;
		ref.mobile_instance_id = item_uid_allocator_next();
		if (!ref.mobile_instance_id || ref.mobile_instance_id == UINT64_MAX)
			return nullptr;
		ref.birth_source = { economic_source_kind::npc_generation, reset_invocation,
				     reset_invocation, 0, slot };
		ref.mobile_vnum = mob_index[rnum].virtual_number;
		ref.birthplace_vnum = world[room].number;
		ref.reset_zone_vnum = zone_table[b->zone].number;
		ref.provenance = quest_mobile_birth_provenance::reset;
		ref.mobile_revision = 1;
		ref.stock_revision = 1;
		b->accepted_usec = accepted_now();
		quest_mobile_native_constructor_digest running_build;
		if (!native_mobile_birth_running_artifact_digest(&running_build) ||
		    !b->mobile.prepare_captured(rnum, REAL, true, running_build, world[room].number,
						shop, &b->constructor))
			return nullptr;
		b->constructor_present = true;
		b->character = b->mobile.character();
		b->runtime_id = b->character->runtime_id;
		b->mobile_bytes = sizeof(*b->character) + sizeof(*b->character->only.npc);
		// General mobile/room hooks still require their actual original owners.
		// The alchemist decision runs later at its original post-reset-tail cut.
		if (world[room].funct ||
		    (IS_SET(b->character->specials.act, ACT_SPEC) && mob_index[rnum].func.mob))
		{
			b->blocked = true;
		}
		const size_t index = available;
		if (index == births.size())
			births.push_back(std::move(b));
		else
			births[index] = std::move(b);
		current_birth = index;
		if (!charge())
		{
			births[index]->blocked = true;
			current_birth = SIZE_MAX;
			discard(index);
			return nullptr;
		}
		if (births[index]->blocked)
			return nullptr;
		return births[index]->character;
	}
	catch (...)
	{
		return nullptr;
	}
}
bool quest_mobile_native_birth_owner::owns(P_char mob) noexcept
{
	return current_birth < births.size() && births[current_birth] &&
	       births[current_birth]->character == mob && !births[current_birth]->mobile_consumed;
}
bool quest_mobile_native_birth_owner::capture_alchemist_spawn(P_char actor, int room) noexcept
{
	if (!reset_in_progress || !nevent_is_game_thread() || !owns(actor) ||
	    room != births[current_birth]->room || room <= NOWHERE || room > top_of_world)
		return false;
	auto &birth = *births[current_birth];
	if (birth.blocked || birth.sealed || !birth.constructor_present ||
	    birth.constructor.wire_version !=
		    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION)
		return false;
	if (!GET_CLASS(actor, CLASS_ALCHEMIST))
	{
		// The genuine original decision returned without attempting a roll.
		birth.non_alchemist_cut_returned = true;
		return true;
	}
	if (birth.alchemist_started || actor->in_room != NOWHERE || !obj_index)
	{
		birth.blocked = true;
		return false;
	}
	birth.alchemist_started = true;
	native_alchemist_vial_choice choice = native_alchemist_vial_choice::not_attempted;
	if (!npc_alchemist_original_birth::capture(actor, room, &choice))
	{
		birth.blocked = true;
		return false;
	}
	birth.alchemist_returned = true;
	birth.constructor.wire_version = NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION;
	birth.constructor.alchemist_choice = static_cast<original_alchemist_choice>(choice);
	birth.constructor.alchemist_grant_uid = 0;
	if (!alchemist_choice_compatible(actor, birth.constructor))
	{
		birth.blocked = true;
		return false;
	}
	if (choice != native_alchemist_vial_choice::selected)
		return true;
	// This is the original read_object(VIRTUAL) no-object branch, before any
	// constructor or UID reservation. An actual preparation failure is distinct.
	const int rnum = real_object(VOBJ_POISON_VIALS);
	if (rnum < 0 || rnum > top_of_objt)
		return true;
	P_obj vial = prepare_item(rnum);
	// Preparation may discard the owner on budget failure; never reuse birth.
	if (!vial || !carry(vial, actor))
		return false;
	if (!owns(actor) || !vial->obj_uid || vial->obj_uid == UINT64_MAX)
		return false;
	auto &current = *births[current_birth];
	current.constructor.alchemist_grant_uid = vial->obj_uid;
	if (!native_mobile_birth_constructor_recipe_valid(current.constructor))
	{
		current.blocked = true;
		return false;
	}
	return true;
}
P_obj quest_mobile_native_birth_owner::prepare_item(int rnum) noexcept
{
	if (current_birth >= births.size() || !births[current_birth] ||
	    births[current_birth]->blocked || rnum < 0 || rnum > top_of_objt)
		return nullptr;
	auto &b = *births[current_birth];
	original_item item;
	try
	{
		if (b.stock.size() >= PLAYER_SNAPSHOT_MAX_ROWS)
		{
			b.blocked = true;
			return nullptr;
		}
		item.stage = std::make_unique<quest_mobile_native_item_stage>();
		b.stock.reserve(b.stock.size() + 1);
		item.uid = item_uid_allocator_next();
		item.rnum = rnum;
		if (!item.uid || !quest_mobile_native_item_stage::prepare(rnum, REAL, item.uid,
									  item.stage.get()))
		{
			b.blocked = true;
			return nullptr;
		}
		item.object = item.stage->object();
		item.effects.resize(item.stage->publication_step_count());
		P_obj result = item.object;
		b.stock.push_back(std::move(item));
		if (!charge())
		{
			b.blocked = true;
			const size_t index = current_birth;
			current_birth = SIZE_MAX;
			discard(index);
			return nullptr;
		}
		return result;
	}
	catch (...)
	{
		if (item.stage)
			item.stage->discard_unadmitted();
		b.blocked = true;
		return nullptr;
	}
}
bool quest_mobile_native_birth_owner::discard_item(P_obj obj) noexcept
{
	if (current_birth >= births.size() || !births[current_birth] || !obj)
		return false;
	auto &b = *births[current_birth];
	for (auto &item : b.stock)
		if (item.object == obj && !item.published)
		{
			if (!quest_mobile_native_local_stock::detach(obj, b.character) ||
			    !item.stage->discard_unadmitted())
			{
				b.blocked = true;
				return false;
			}
			item.stage.reset();
			item.object = nullptr;
			item.rnum = -1;
			item.effects.clear();
			charge();
			return true;
		}
	return false;
}
bool quest_mobile_native_birth_owner::carry(P_obj obj, P_char mob) noexcept
{
	if (!owns(mob))
		return false;
	const bool placed = quest_mobile_native_local_stock::carry(obj, mob);
	if (!placed)
		births[current_birth]->blocked = true;
	return placed;
}
bool quest_mobile_native_birth_owner::equip(P_obj obj, P_char mob, int slot) noexcept
{
	if (!owns(mob))
		return false;
	const bool placed = quest_mobile_native_local_stock::equip(obj, mob, slot);
	if (!placed)
		births[current_birth]->blocked = true;
	return placed;
}
bool quest_mobile_native_birth_owner::nest(P_obj obj, P_obj target, P_char mob) noexcept
{
	if (!owns(mob) || births[current_birth]->blocked)
		return false;
	auto &b = *births[current_birth];
	bool placed = false;
	if (target && target->type == ITEM_CONTAINER && target->value[4] > 0)
	{
		const auto selected = std::find_if(
			b.stock.begin(), b.stock.end(), [target](const auto &item)
			{ return item.object == target && item.stage && !item.published; });
		if (selected != b.stock.end())
		{
			quest_mobile_native_container_shell shell;
			// Actual original grouping/full weight precede read/cleanup; the
			// original correction follows. A failed leg blocks this same owner.
			placed = quest_mobile_native_local_stock::begin_reducing_nest(obj, target,
										      mob) &&
				 selected->stage->capture_container_shell(&shell) &&
				 quest_mobile_native_local_stock::finish_reducing_nest(obj, target,
										       mob, shell);
		}
	}
	else
		placed = quest_mobile_native_local_stock::nest(obj, target, mob);
	if (!placed)
		b.blocked = true;
	return placed;
}
bool quest_mobile_native_birth_owner::original_object(int rnum, P_obj *selected) noexcept
{
	if (!selected)
		return false;
	P_obj target = nullptr;
	bool pending = false;
	if (!quest_mobile_native_item_stage::original_reset_target(rnum, &target, &pending))
		return false;
	if (pending)
	{
		if (!reset_in_progress || current_birth >= births.size() || !births[current_birth])
			return false;
		const auto &birth = *births[current_birth];
		if (birth.blocked || birth.retired || birth.mobile_consumed ||
		    birth.zone != reset_zone_rnum)
			return false;
		const auto item = std::find_if(
			birth.stock.begin(), birth.stock.end(),
			[target, rnum](const auto &candidate)
			{
				return candidate.object == target && candidate.stage &&
				       !candidate.published && candidate.rnum == rnum &&
				       candidate.uid == target->obj_uid && target->R_num == rnum &&
				       candidate.stage->owns_pending_original_target(target);
			});
		if (item == birth.stock.end())
			return false; // The genuine newest target needs its foreign owner.
	}
	*selected = target;
	return true;
}
void quest_mobile_native_birth_owner::skipped_item(P_char mob, P_obj missing) noexcept
{
	int vnum = 0;
	if (!owns(mob) || !quest_mobile_native_reset_material_owner::select(mob, missing, &vnum))
		return;
	const int rnum = real_object(vnum);
	if (rnum < 0)
	{
		logit(LOG_DEBUG,
		      "enhance: missing high-quality material %d for skipped NPC item %d.", vnum,
		      OBJ_VNUM(missing));
		return;
	}
	P_obj material = prepare_item(rnum);
	if (material)
		carry(material, mob);
}
void quest_mobile_native_birth_owner::block_mobile() noexcept
{
	if (current_birth < births.size() && births[current_birth])
		births[current_birth]->blocked = true;
	seal_mobile();
}
void quest_mobile_native_birth_owner::seal_mobile() noexcept
{
	if (current_birth >= births.size() || !births[current_birth])
		return;
	const size_t index = current_birth;
	auto &b = *births[index];
	current_birth = SIZE_MAX;
	if (b.blocked || b.sealed || !b.character)
		return;
	if (GET_CLASS(b.character, CLASS_ALCHEMIST) &&
	    (!b.alchemist_started || !b.alchemist_returned ||
	     b.constructor.wire_version !=
		     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION))
	{
		b.blocked = true;
		return;
	}
	try
	{
		if (quest_mobile_native_capture(b.character, b.reference,
						quest_mobile_lifetime_state::live,
						b.reference.birth_operation, 1,
						&b.image) != player_snapshot_capture_result::ok)
		{
			b.blocked = true;
			return;
		}
		// A constructed orphan cannot be published outside this exact born forest.
		size_t selected = 0;
		for (const auto &item : b.stock)
			if (item.stage)
			{
				++selected;
				size_t found = 0;
				for (const auto &row : b.image.items)
					if (row.object_uid == item.uid)
						++found;
				if (found != 1)
				{
					b.blocked = true;
					return;
				}
			}
		if (selected != b.image.items.size())
		{
			b.blocked = true;
			return;
		}
		b.current_custody.resize(b.image.items.size());
		b.recipes.resize(b.image.items.size());
		for (size_t row = 0; row < b.image.items.size(); ++row)
		{
			const auto selected =
				std::find_if(b.stock.begin(), b.stock.end(), [&](const auto &item)
					     { return item.uid == b.image.items[row].object_uid; });
			if (selected == b.stock.end() || !selected->stage ||
			    !selected->stage->capture_recipe(b.image.items[row], &b.recipes[row]))
			{
				b.blocked = true;
				return;
			}
		}
		if (!native_mobile_birth_recipe_valid(b.image.items, b.recipes))
		{
			b.blocked = true;
			return;
		}
		// NBC4 requires the complete original decision capsule. Only a fresh
		// birth which genuinely reached the non-alchemist return cut may record
		// not_attempted; historical NBC2 commands are never rewritten.
		if (b.constructor.wire_version ==
		    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION)
		{
			if (!b.non_alchemist_cut_returned ||
			    GET_CLASS(b.character, CLASS_ALCHEMIST) ||
			    b.constructor.alchemist_choice !=
				    original_alchemist_choice::not_attempted ||
			    b.constructor.alchemist_grant_uid)
			{
				b.blocked = true;
				return;
			}
			b.constructor.wire_version =
				NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION;
		}
		if (!native_mobile_birth_cash_role_recipe_capture(b.constructor, &b.cash_role))
		{
			b.blocked = true;
			return;
		}
		b.cash_role_present = true;
		if (b.cash_role.role == native_mobile_birth_cash_role::shared_shopkeeper)
		{
			// Genuine source/stage cut only; the earlier accepted_usec is not a
			// save-time observation. The clock is sampled once at this final cut.
			if (!reset_dispatch_identity() || b.zone != reset_zone_rnum ||
			    b.reference.birth_source.source.bytes != reset_invocation.bytes ||
			    b.reference.birth_source.generation.bytes != reset_invocation.bytes ||
			    b.shared_checkpoint || !shared_cash_role_current(b))
			{
				b.blocked = true;
				return;
			}
			b.shared_saved_at =
				std::chrono::duration_cast<std::chrono::seconds>(
					std::chrono::system_clock::now().time_since_epoch())
					.count();
			if (b.shared_saved_at < 0)
			{
				b.blocked = true;
				return;
			}
			// The original owner freezes the complete final source/body/choice
			// cut before fallible passive capture. Detached, unlinked and
			// unscheduled stage ownership survives a pure allocation refusal.
			b.shared_source_cut = true;
		}
		else if (b.cash_role.role != native_mobile_birth_cash_role::ordinary_wallet)
		{
			b.blocked = true;
			return;
		}
		std::vector<quest_mobile_native_item_binding> inputs;
		inputs.reserve(b.stock.size());
		for (const auto &item : b.stock)
			if (item.stage)
				inputs.push_back(item.stage->binding_input());
		if (!shop_trade_original_procedure_binding_stage::prepare_native_birth(inputs,
										       b.bindings))
		{
			b.blocked = true;
			return;
		}
		b.sealed = true;
		if (b.shared_source_cut)
			capture_shared_checkpoint(index);
		if (!charge())
		{
			b.blocked = true;
			discard(index);
		}
	}
	catch (...)
	{
		b.blocked = true;
	}
}
bool quest_mobile_native_birth_owner::capture_shared_checkpoint(size_t index) noexcept
{
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	if (b.shared_checkpoint)
		return true;
	if (!b.shared_source_cut || b.shared_saved_at < 0 || b.cold || b.blocked || b.submitted ||
	    !b.constructor_present || !shared_cash_role_current(b) || b.mobile_consumed ||
	    !b.character || b.mobile.character() != b.character ||
	    b.character->runtime_id != b.runtime_id || find_character_by_runtime_id(b.runtime_id) ||
	    b.character->in_room != NOWHERE)
		return false;
	try
	{
		quest_mobile_native_image observed;
		flatfile_shopkeeper_record record;
		std::vector<uint8_t> checkpoint;
		if (quest_mobile_native_capture(b.character, b.reference,
						quest_mobile_lifetime_state::live,
						b.reference.birth_operation, 1,
						&observed) != player_snapshot_capture_result::ok ||
		    !same_image(b.image, observed) ||
		    b.mobile.capture_shopkeeper_checkpoint(
			    b.character, b.runtime_id, b.room, b.shop, b.reference, b.constructor,
			    b.cash_role, b.shared_saved_at,
			    &record) != player_snapshot_capture_result::ok ||
		    !flatfile_shopkeeper_initial_checkpoint_encode(record, &checkpoint))
			return false;
		auto captured = std::make_unique<const std::vector<uint8_t>>(std::move(checkpoint));
		b.shared_checkpoint = std::move(captured);
		if (!charge())
		{
			b.shared_checkpoint.reset();
			charge();
			return false;
		}
		return true;
	}
	catch (...)
	{
		// Pure capture/codec allocation retry only. No constructor/effect/clock
		// rerun, original decision change, stage publication or new source cut.
		return false;
	}
}
bool quest_mobile_native_birth_owner::prepare_shared_capture(size_t index) noexcept
{
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	if (!economic_gameplay_authority::active_regular_sql() || b.cold || b.blocked ||
	    !b.sealed || b.submitted || !b.constructor_present || !b.shared_source_cut ||
	    b.shared_saved_at < 0 || !shared_cash_role_current(b) || b.mobile_consumed ||
	    !b.character || b.mobile.character() != b.character ||
	    b.character->runtime_id != b.runtime_id || find_character_by_runtime_id(b.runtime_id) ||
	    b.character->in_room != NOWHERE)
		return false;
	if (!capture_shared_checkpoint(index) || !b.shared_checkpoint ||
	    b.shared_checkpoint->empty())
		return false;
	try
	{
		quest_mobile_native_image observed;
		if (quest_mobile_native_capture(b.character, b.reference,
						quest_mobile_lifetime_state::live,
						b.reference.birth_operation, 1,
						&observed) != player_snapshot_capture_result::ok ||
		    !same_image(b.image, observed))
			return false;
		if (b.canonical.empty())
		{
			critical_command command;
			if (economic_gameplay_authority::prepare_native_mobile_birth_shared_shopkeeper(
				    b.image, b.recipes, b.cash_role,
				    critical_source_site::zone_event, b.accepted_usec,
				    &command) != economic_accounting_error::ok)
				return false;
			std::vector<uint8_t> canonical;
			if (critical_command_encode(command, &canonical) !=
			    critical_command_codec_result::ok)
				return false;
			b.command = std::move(command);
			b.canonical = std::move(canonical);
			if (!charge())
			{
				b.command = {};
				std::vector<uint8_t>{}.swap(b.canonical);
				charge();
				return false;
			}
		}
		if (b.envelope.attachment.empty())
		{
			native_mobile_birth_recovery_context context;
			context.items.reserve(b.stock.size());
			for (const auto &item : b.stock)
			{
				native_mobile_birth_recovery_item row;
				row.object_uid = item.uid;
				row.effects.resize(item.effects.size());
				context.items.push_back(std::move(row));
			}
			critical_native_recovery_envelope envelope;
			envelope.command = b.command;
			envelope.revision = 1;
			envelope.phase = critical_native_recovery_phase::execution_pending;
			if (birth_recovery_encode(b.command, context, &envelope.attachment,
						  b.shared_checkpoint.get()) !=
				    economic_accounting_error::ok ||
			    !native_mobile_birth_shared_shop_recovery_initial(envelope))
				return false;
			b.envelope = std::move(envelope);
			b.recovery = std::move(context);
			if (!charge())
			{
				b.envelope = {};
				b.recovery = {};
				return false;
			}
		}
		return native_mobile_birth_shared_shop_recovery_initial(b.envelope);
	}
	catch (...)
	{
		return false;
	}
}
void quest_mobile_native_birth_owner::finish_reset() noexcept
{
	if (!reset_in_progress)
		return;
	if (!reset_dispatch.completed || !reset_dispatch.valid)
	{
		zone_reset_item_owner::abort_warm_capture();
		reset_dispatch.held = true;
		reset_dispatch.retryable = false;
		// No S was reached: retain invocation, original locals and current factory.
		// A genuine explicit pure hold uses hold_reset and does not call finish.
		charge();
		return;
	}
	// The real stop releases borrowed mobile lifetimes before sealing its final
	// owner. All original slot/body/decision state survives until this boundary.
	for (auto &hold : reset_dispatch.actors)
		hold = {};
	seal_mobile();
	reset_in_progress = false;
	reset_zone_rnum = -1;
	reset_invocation = {};
	reset_dispatch = {};
	charge();
}
bool quest_mobile_native_birth_owner::discard(size_t index) noexcept
{
	if (index >= births.size() || !births[index])
		return false;
	auto &b = *births[index];
	if (b.submitted || b.mobile_started || b.cold || reset_holds_birth(b))
		return false;
	for (auto i = b.stock.rbegin(); i != b.stock.rend(); ++i)
		if (i->stage)
		{
			if (!quest_mobile_native_local_stock::detach(i->object, b.character) ||
			    !i->stage->discard_unadmitted())
				return false;
			i->stage.reset();
		}
	if (b.character)
	{
		GET_CARRYING_W(b.character) = 0;
		IS_CARRYING_N(b.character) = 0;
		if (!b.mobile.discard_empty())
			return false;
	}
	births[index].reset();
	charge();
	return true;
}

bool quest_mobile_native_birth_owner::cleanup_refusal(const critical_command &command,
						      const critical_completion &completion,
						      void *context) noexcept
{
	if (!context || !nevent_is_game_thread())
		return false;
	auto &b = *static_cast<original_birth *>(context);
	try
	{
		std::vector<uint8_t> canonical;
		if (reset_holds_birth(b) || !b.completed ||
		    !same_completion(b.completion, completion) ||
		    critical_command_encode(command, &canonical) !=
			    critical_command_codec_result::ok ||
		    canonical != b.canonical || b.cold || b.mobile_started || b.mobile_consumed ||
		    b.binding_committed)
			return false;
		if (b.refusal_cleanup_returned)
			return true;
		if (b.refusal_cleanup_started)
			return false;
		for (const auto &item : b.stock)
			if (item.published || item.enrollment_started)
				return false;
		// The guarded original refusal owns this definite unadmitted disposal.
		// Keep command/receipt and returned fact until the coordinator reproof settles.
		b.refusal_cleanup_started = true;
		for (auto item = b.stock.rbegin(); item != b.stock.rend(); ++item)
			if (item->stage)
			{
				if (!quest_mobile_native_local_stock::detach(item->object,
									     b.character) ||
				    !item->stage->discard_unadmitted())
					return false;
				item->stage.reset();
				item->object = nullptr;
			}
		if (b.character)
		{
			GET_CARRYING_W(b.character) = 0;
			IS_CARRYING_N(b.character) = 0;
			if (!b.mobile.discard_empty())
				return false;
			b.character = nullptr;
			b.mobile_bytes = 0;
		}
		b.refusal_cleanup_returned = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_native_birth_owner::settle_checkpoint(size_t index) noexcept
{
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	if (shared_birth(b) && (!shared_shop_command(b.command) || !b.shared_checkpoint ||
				!birth_recovery_valid(b.envelope)))
		return false;
	if (!b.checkpoint)
		return true;
	// False retains both entire writer sides. No absence scan or health flag retries an effect.
	if (!critical_native_mobile_birth_publication_owner::checkpoint_context(
		    b.checkpoint->expected, b.checkpoint->successor))
		return false;
	b.envelope = std::move(b.checkpoint->successor);
	b.recovery = std::move(b.checkpoint->context);
	// Release only the original choice owned by this successfully settled writer.
	// Encoding or CAS refusal retains it; a later scheduling step must choose anew.
	if (b.checkpoint->consumes_choice)
		b.chosen_context.reset();
	b.checkpoint.reset();
	// No allocation/encoding after durable CAS. Existing aggregate reserve remains conservative.
	return true;
}
bool quest_mobile_native_birth_owner::checkpoint(
	size_t index, const native_mobile_birth_recovery_context &next) noexcept
{
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	if (b.checkpoint)
		return settle_checkpoint(index);
	if (b.envelope.revision == UINT64_MAX)
		return false;
	try
	{
		auto writer = std::make_unique<original_birth_checkpoint>();
		writer->expected = b.envelope;
		writer->successor = b.envelope;
		++writer->successor.revision;
		writer->context = next;
		writer->consumes_choice = &next == b.chosen_context.get();
		if (birth_recovery_encode(b.command, next, &writer->successor.attachment,
					  b.shared_checkpoint.get()) !=
			    economic_accounting_error::ok ||
		    !birth_recovery_successor(writer->expected, writer->successor))
			return false;
		b.checkpoint = std::move(writer);
		if (!charge())
		{
			b.checkpoint.reset();
			return false;
		}
		return settle_checkpoint(index);
	}
	catch (...)
	{
		return false;
	}
}
int quest_mobile_native_birth_owner::prepare_action(size_t index, uint8_t kind, size_t row,
						    size_t step) noexcept
{
	if (!settle_checkpoint(index))
		return -1;
	auto &b = *births[index];
	try
	{
		const auto current = action_value(b.recovery, kind, row, step);
		if (current.returned)
			return current.succeeded ? 0 : -1;
		if (current.started)
			return b.action_not_attempted && b.pending_action == kind &&
					       b.pending_row == row && b.pending_step == step ?
				       1 :
				       -1;
		if (b.action_not_attempted || b.envelope.revision > UINT64_MAX - 2)
			return -1;
		native_mobile_birth_recovery_context started = b.recovery;
		set_action(started, kind, row, step, { true, false, false, false });
		auto intent = std::make_unique<original_birth_checkpoint>();
		intent->expected = b.envelope;
		intent->successor = b.envelope;
		++intent->successor.revision;
		intent->context = started;
		if (birth_recovery_encode(b.command, started, &intent->successor.attachment,
					  b.shared_checkpoint.get()) !=
			    economic_accounting_error::ok ||
		    !birth_recovery_successor(intent->expected, intent->successor))
			return -1;
		// Every possible genuine returned outcome is preallocated before intent I/O.
		std::array<std::unique_ptr<original_birth_checkpoint>, 4> returned;
		for (size_t bits = 0; bits < returned.size(); ++bits)
		{
			if ((bits & 2) && kind != ITEM_EFFECT &&
			    !(kind == MOBILE_EFFECT && step == 3))
				continue;
			auto writer = std::make_unique<original_birth_checkpoint>();
			writer->expected = intent->successor;
			writer->successor = intent->successor;
			++writer->successor.revision;
			writer->context = started;
			set_action(writer->context, kind, row, step,
				   { true, true, static_cast<bool>(bits & 1),
				     static_cast<bool>(bits & 2) });
			if (birth_recovery_encode(
				    b.command, writer->context, &writer->successor.attachment,
				    b.shared_checkpoint.get()) == economic_accounting_error::ok &&
			    birth_recovery_successor(writer->expected, writer->successor))
				returned[bits] = std::move(writer);
		}
		if (!returned[1] && !returned[3])
			return -1;
		b.checkpoint = std::move(intent);
		b.returned_writers = std::move(returned);
		b.pending_action = kind;
		b.pending_row = row;
		b.pending_step = step;
		b.action_not_attempted = true;
		if (!charge())
		{
			b.checkpoint.reset();
			for (auto &writer : b.returned_writers)
				writer.reset();
			b.action_not_attempted = false;
			return -1;
		}
		return settle_checkpoint(index) ? 1 : -1;
	}
	catch (...)
	{
		return -1;
	}
}
bool quest_mobile_native_birth_owner::finish_action(
	size_t index, const native_mobile_birth_recovery_effect &actual) noexcept
{
	auto &b = *births[index];
	if (!actual.started)
		return false; // In-process original owner knows the effect was not attempted.
	b.action_not_attempted = false;
	if (!actual.returned)
	{
		b.blocked = true;
		return false; // Actual started/unreturned remains durable uncertainty.
	}
	const size_t bits = static_cast<size_t>(actual.succeeded) |
			    (static_cast<size_t>(actual.periodic) << 1);
	if (!b.returned_writers[bits])
	{
		b.blocked = true;
		return false;
	}
	b.checkpoint = std::move(b.returned_writers[bits]);
	for (auto &writer : b.returned_writers)
		writer.reset();
	// The exact returned writer survives uncertainty and settles before any next effect.
	return settle_checkpoint(index) && actual.succeeded;
}
// This cleanup releases only private reconstruction/adoption metadata. It
// never extracts a published body, replays a callback or erases a durable frame.
bool quest_mobile_native_birth_owner::clear_cold_projection(size_t index) noexcept
{
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	if (!b.cold || b.cold_rebuilding)
		return false;
	if (b.cold_adopted)
	{
		for (auto &item : b.stock)
		{
			if (item.stage)
			{
				if (item.adopted_metadata && !item.stage->abandon_adoption())
					return false;
				item.stage.reset();
			}
			item.object = nullptr;
			item.adopted_metadata = false;
			item.published = item.enrolled = false;
			item.enrollment_started = item.enrollment_returned = false;
			std::vector<quest_mobile_native_item_effect>{}.swap(item.effects);
		}
		b.cold_adopted = false;
		return true;
	}
	b.bindings = shop_trade_original_procedure_binding_stage{};
	// The image, not construction order, proves leaves precede their parents
	// during removal. Native allocation owns every node until exact transfer.
	for (auto row = b.image.items.rbegin(); row != b.image.items.rend(); ++row)
	{
		auto item = std::find_if(b.stock.begin(), b.stock.end(), [&](const auto &value)
					 { return value.uid == row->object_uid; });
		if (item == b.stock.end())
			return false;
		if (item->stage)
		{
			if ((b.character &&
			     !quest_mobile_native_local_stock::detach(item->object, b.character)) ||
			    !item->stage->discard_unadmitted())
				return false;
			item->stage.reset();
			item->object = nullptr;
		}
		std::vector<quest_mobile_native_item_effect>{}.swap(item->effects);
	}
	if (b.character)
	{
		GET_CARRYING_W(b.character) = 0;
		IS_CARRYING_N(b.character) = 0;
		if (!b.mobile.discard_empty())
			return false;
		b.character = nullptr;
	}
	b.runtime_id = 0;
	b.mobile_bytes = 0;
	return true;
}

// Only a projection retained by this original owner can enter this retry path.
// Fresh SQL/source/cash/custody proof and confirmed rollback precede every call.
// Historical returned facts authorize rebuilding process-local services; they
// never become new gameplay callbacks or new journal returned observations.
bool quest_mobile_native_birth_owner::resume_cold_projection(size_t index) noexcept
{
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	if (!b.cold || !b.cold_rebuilding || !b.character || !b.runtime_id)
		return false;
	try
	{
		// Cover every retained historical prefix, including a previous pure
		// preparation budget refusal. No item/room/AF/event service precedes
		// the complete prospective reservation on this retry.
		if (b.shared_cold_affects && !charge())
			return false;
		if (b.mobile.publication_consumed_)
		{
			if (find_character_by_runtime_id(b.runtime_id) != b.character ||
			    b.mobile.publication_runtime_id_ != b.runtime_id)
				return false;
		}
		else if (b.mobile.character() != b.character ||
			 find_character_by_runtime_id(b.runtime_id))
			return false;
		// Reject list corruption and foreign incarnations before inspecting the
		// exact stage-owned graph. Partial restored ownership is not absence.
		for (P_char slow = character_list, fast = character_list; fast && fast->next;)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
				return false;
		}
		for (P_obj slow = object_list, fast = object_list; fast && fast->next;)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
				return false;
		}
		for (P_char actual = character_list; actual; actual = actual->next)
		{
			if (actual == b.character)
				continue;
			quest_mobile_native_reference reference;
			const auto *binding = actual->native_mobile_binding.encoded_reference_;
			if (std::all_of(binding, binding + QUEST_MOBILE_NATIVE_REFERENCE_BYTES,
					[](uint8_t value) { return value == 0; }))
				continue;
			if (quest_mobile_native_reference_decode(
				    { binding, QUEST_MOBILE_NATIVE_REFERENCE_BYTES }, &reference) !=
				    player_snapshot_codec_result::ok ||
			    reference.mobile_instance_id == b.reference.mobile_instance_id ||
			    reference.birth_operation.bytes == b.reference.birth_operation.bytes)
				return false;
		}
		for (const auto &item : b.stock)
		{
			size_t matches = 0;
			for (P_obj actual = object_list; actual; actual = actual->next)
				if (actual->obj_uid == item.uid)
				{
					if (actual != item.object || !item.published)
						return false;
					++matches;
				}
			if (matches != (item.published ? 1U : 0U))
				return false;
		}
		quest_mobile_native_image observed;
		if (quest_mobile_native_capture(b.character, b.reference,
						quest_mobile_lifetime_state::live,
						b.reference.birth_operation, 1,
						&observed) != player_snapshot_capture_result::ok ||
		    !same_image(observed, b.image))
			return false;
		if (b.shared_cold_affects && !b.recovery.mobile_effects[1].succeeded &&
		    (!charge() ||
		     !b.mobile.restore_shared_shopkeeper_affects_before_room(b.character) ||
		     !charge() || !shared_keeper_checkpoint_current(b, b.character, b.runtime_id)))
			return false;
		for (size_t row = 0; row < b.stock.size(); ++row)
		{
			auto &item = b.stock[row];
			const auto &saved = b.recovery.items[row];
			if (!saved.publication.succeeded || item.published)
				continue;
			if (!item.stage)
				return false;
			item.stage->retain_admitted();
			if (item.stage->publish() != item.object)
				return false;
			item.published = true;
		}
		if (b.recovery.mobile_effects[0].succeeded)
		{
			P_char actual = nullptr;
			const bool restored =
				b.mobile.restore_published(b.room, b.recovery.mobile_effects,
							   b.recovery.mobile_choices, &actual);
			// Stage ownership is consumed before some service allocations return.
			// Retain the actual partial actor even when this attempt refuses.
			b.mobile_started = b.mobile.publication_consumed_;
			b.mobile_consumed = b.mobile.publication_consumed_;
			if (!restored || actual != b.character ||
			    find_character_by_runtime_id(b.runtime_id) != b.character)
				return false;
		}
		for (size_t row = 0; row < b.stock.size(); ++row)
		{
			auto &item = b.stock[row];
			const auto &saved = b.recovery.items[row];
			if (!saved.enrollment.succeeded || item.enrolled)
				continue;
			const auto literal = std::find_if(b.image.items.begin(),
							  b.image.items.end(),
							  [&](const auto &value)
							  { return value.object_uid == item.uid; });
			if (literal == b.image.items.end() || !item.stage ||
			    !quest_mobile_native_local_stock::restore_enrollment(item.object,
										 b.character) ||
			    !item.stage->rebuild_enrollment(
				    item.object, b.recipes[literal - b.image.items.begin()],
				    { saved.next_step, saved.current_step_started, saved.admitted,
				      saved.published },
				    item.effects))
				return false;
			item.enrolled = true;
			item.enrollment_started = true;
			item.enrollment_returned = true;
		}
		if (b.shared_cold_affects &&
		    (!charge() || !shared_keeper_checkpoint_current(b, b.character, b.runtime_id)))
			return false;
		// All historical services now have actual restored ownership. Future
		// original pending actions still use their unchanged journal writers.
		b.cold_rebuilding = false;
		b.cold = false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_native_birth_owner::observe_current_published_identity(
	P_char actor, uint64_t runtime, bool *present,
	quest_mobile_native_reference *output) noexcept
{
	if (!nevent_is_game_thread() || !actor || !runtime || !present || !output ||
	    find_character_by_runtime_id(runtime) != actor || actor->runtime_id != runtime)
		return false;
	const auto &storage = actor->native_mobile_binding.encoded_reference_;
	if (std::all_of(std::begin(storage), std::end(storage),
			[](uint8_t byte) { return byte == 0; }))
	{
		const auto &binding = actor->native_mobile_binding;
		const auto zero = [](const auto &bytes) noexcept
		{
			return std::all_of(std::begin(bytes), std::end(bytes),
					   [](uint8_t value) { return value == 0; });
		};
		// A missing reference with nonzero cash metadata is corrupt/mixed
		// storage, never authority to treat the native lifetime as absent.
		if (!zero(binding.cash_revision_) || !zero(binding.wallet_mapping_id_) ||
		    !zero(binding.lineage_) || !zero(binding.birth_epoch_))
			return false;
		*present = false;
		return true;
	}
	quest_mobile_native_reference candidate;
	if (!quest_mobile_native_reference_copy(actor, runtime, &candidate))
		return false;
	*output = candidate;
	*present = true;
	return true;
}

bool quest_mobile_native_birth_owner::install_current_published_metadata(
	P_char actor, uint64_t runtime, const quest_mobile_native_image &image,
	const native_mobile_wallet_origin &wallet) noexcept
{
#ifdef __NO_MYSQL__
	(void)actor;
	(void)runtime;
	(void)image;
	(void)wallet;
	return false;
#else
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_sql_recovery() ||
	    !actor || !runtime || actor->runtime_id != runtime || !IS_NPC(actor) ||
	    !actor->only.npc || image.state != quest_mobile_lifetime_state::live || !image.cash ||
	    !image.cash->revision || !wallet.wallet_mapping_id ||
	    wallet.mobile_instance_id != image.reference.mobile_instance_id ||
	    wallet.birth_operation.bytes != image.reference.birth_operation.bytes ||
	    critical_operation_id_is_zero(wallet.lineage) ||
	    critical_operation_id_is_zero(wallet.birth_epoch))
		return false;
	P_char indexed = find_character_by_runtime_id(runtime);
	if (indexed && indexed != actor)
		return false;
	try
	{
		// This private delegation is called only for the world's genuinely
		// retained detached stage, or the same indexed body on an exact retry.
		// Complete CURRENT forest and literal cash precede every metadata write.
		quest_mobile_native_image observed;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
		if (quest_mobile_native_reference_encode(image.reference, &reference) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_capture(actor, image.reference, image.state,
						image.last_transition_operation,
						image.cash->revision,
						&observed) != player_snapshot_capture_result::ok ||
		    !same_image(image, observed))
			return false;
		auto &binding = actor->native_mobile_binding;
		const auto matches_or_zero = [](const auto &stored, const auto &expected) noexcept
		{
			return std::equal(std::begin(stored), std::end(stored), expected.begin()) ||
			       std::all_of(std::begin(stored), std::end(stored),
					   [](uint8_t value) { return value == 0; });
		};
		std::array<uint8_t, QUEST_MOBILE_NATIVE_CASH_BINDING_METADATA_BYTES> exact{},
			current{};
		const auto put = [](uint8_t *out, uint64_t value) noexcept
		{
			for (size_t i = 0; i < 8; ++i)
				out[i] = static_cast<uint8_t>(value >> (8 * i));
		};
		put(exact.data(), image.cash->revision);
		put(exact.data() + 8, wallet.wallet_mapping_id);
		std::copy(wallet.lineage.bytes.begin(), wallet.lineage.bytes.end(),
			  exact.begin() + 16);
		std::copy(wallet.birth_epoch.bytes.begin(), wallet.birth_epoch.bytes.end(),
			  exact.begin() + 32);
		std::copy(std::begin(binding.cash_revision_), std::end(binding.cash_revision_),
			  current.begin());
		std::copy(std::begin(binding.wallet_mapping_id_),
			  std::end(binding.wallet_mapping_id_), current.begin() + 8);
		std::copy(std::begin(binding.lineage_), std::end(binding.lineage_),
			  current.begin() + 16);
		std::copy(std::begin(binding.birth_epoch_), std::end(binding.birth_epoch_),
			  current.begin() + 32);
		if (!matches_or_zero(binding.encoded_reference_, reference) ||
		    (current != exact && std::any_of(current.begin(), current.end(),
						     [](uint8_t value) { return value != 0; })))
			return false;
		// All fallible observations precede this nonallocating exact install.
		std::copy(reference.begin(), reference.end(), binding.encoded_reference_);
		std::copy_n(exact.begin(), 8, binding.cash_revision_);
		std::copy_n(exact.begin() + 8, 8, binding.wallet_mapping_id_);
		std::copy_n(exact.begin() + 16, 16, binding.lineage_);
		std::copy_n(exact.begin() + 32, 16, binding.birth_epoch_);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_mobile_native_birth_owner::restore_current_published_enrollment(P_obj object,
									   P_char actor) noexcept
{
	return nevent_is_game_thread() && economic_gameplay_authority::active_sql_recovery() &&
	       quest_mobile_native_local_stock::restore_enrollment(object, actor);
}

bool quest_mobile_native_birth_owner::restore_current_published_constructor_policy(
	P_char actor, const quest_mobile_native_constructor_recipe &recipe) noexcept
{
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_sql_recovery() ||
	    !actor || !IS_NPC(actor) || !actor->only.npc || !world || !mob_index ||
	    !native_mobile_birth_constructor_recipe_valid(recipe) || GET_RNUM(actor) < 0 ||
	    GET_RNUM(actor) > top_of_mobt ||
	    mob_index[GET_RNUM(actor)].virtual_number != recipe.mobile_vnum ||
	    (recipe.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
	     recipe.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION))
		return false;
	try
	{
		const int room = real_room(recipe.reset_room_vnum);
		quest_mobile_native_constructor_digest build{}, procedure{}, tail{};
		if (room < 0 || room > top_of_world ||
		    !native_mobile_birth_running_artifact_digest(&build) ||
		    build != recipe.build_digest ||
		    !native_mobile_birth_procedure_capture(recipe.mobile_vnum, build, &procedure) ||
		    procedure != recipe.procedure_after ||
		    !native_mobile_birth_reset_tail_capture(recipe.mobile_vnum,
							    recipe.reset_room_vnum,
							    recipe.reset_shop_index, &tail) ||
		    tail != recipe.reset_tail || world[room].funct ||
		    (IS_SET(actor->specials.act, ACT_SPEC) && mob_index[GET_RNUM(actor)].func.mob))
			return false;
		GET_BIRTHPLACE(actor) = recipe.reset_room_vnum;
		apply_zone_modifier(actor);
		if (recipe.reset_shop_index >= 0)
			bind_shopkeeper(actor, recipe.reset_shop_index);
		return alchemist_choice_compatible(actor, recipe) &&
		       (recipe.wire_version !=
				NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION ||
			npc_alchemist_original_birth::restore_latch(
				actor, static_cast<native_alchemist_vial_choice>(
					       recipe.alchemist_choice)));
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_native_birth_owner::recover_cold(size_t index, bool allow_reconstruction) noexcept
{
#ifdef __NO_MYSQL__
	(void)index;
	(void)allow_reconstruction;
	return false;
#else
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	const bool shared = shared_birth(b);
	flatfile_shopkeeper_record original_checkpoint;
	if (shared && (!shared_shop_command(b.command) || !b.shared_checkpoint ||
		       !shared_cash_role_current(b) || !birth_recovery_valid(b.envelope) ||
		       !flatfile_shopkeeper_initial_checkpoint_decode(*b.shared_checkpoint,
								      &original_checkpoint)))
		return false;
	const auto install_original_cash_metadata = [&]() noexcept
	{
		if (!b.character || !b.image.cash || !b.completed ||
		    b.completion.disposition != critical_completion_disposition::execution ||
		    (b.completion.outcome != critical_apply_outcome::applied &&
		     b.completion.outcome != critical_apply_outcome::already_applied))
			return false;
		birth_wallet_receipt_values result;
		economic_frozen_intent intent;
		if (!decode_birth_wallet_receipt(b.command, b.completion, &result) ||
		    economic_intent_decode(b.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(b.command, intent) !=
			    economic_accounting_error::ok ||
		    result.mobile_instance_id != b.reference.mobile_instance_id ||
		    result.cash_revision != b.image.cash->revision ||
		    (shared ? result.wallet_mapping_id != 0 : !result.wallet_mapping_id) ||
		    critical_operation_id_is_zero(intent.admission.metadata.lineage) ||
		    critical_operation_id_is_zero(intent.admission.metadata.epoch))
			return false;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_CASH_BINDING_METADATA_BYTES> exact{},
			current{};
		const auto put = [](uint8_t *out, uint64_t value) noexcept
		{
			for (size_t i = 0; i < 8; ++i)
				out[i] = static_cast<uint8_t>(value >> (8 * i));
		};
		put(exact.data(), result.cash_revision);
		put(exact.data() + 8, result.wallet_mapping_id);
		std::copy(intent.admission.metadata.lineage.bytes.begin(),
			  intent.admission.metadata.lineage.bytes.end(), exact.begin() + 16);
		std::copy(intent.admission.metadata.epoch.bytes.begin(),
			  intent.admission.metadata.epoch.bytes.end(), exact.begin() + 32);
		auto &binding = b.character->native_mobile_binding;
		std::copy(std::begin(binding.cash_revision_), std::end(binding.cash_revision_),
			  current.begin());
		std::copy(std::begin(binding.wallet_mapping_id_),
			  std::end(binding.wallet_mapping_id_), current.begin() + 8);
		std::copy(std::begin(binding.lineage_), std::end(binding.lineage_),
			  current.begin() + 16);
		std::copy(std::begin(binding.birth_epoch_), std::end(binding.birth_epoch_),
			  current.begin() + 32);
		if (current != exact && std::any_of(current.begin(), current.end(),
						    [](uint8_t byte) { return byte != 0; }))
			return false;
		// Invoked only after this original owner's genuine SQL/image/source cut.
		// Installs observed runtime metadata; never writes a wallet or issues IDs.
		std::copy_n(exact.begin(), 8, binding.cash_revision_);
		std::copy_n(exact.begin() + 8, 8, binding.wallet_mapping_id_);
		std::copy_n(exact.begin() + 16, 16, binding.lineage_);
		std::copy_n(exact.begin() + 32, 16, binding.birth_epoch_);
		return true;
	};

	if (!b.cold)
		return true;
	if (shared ? !shared_cash_role_current(b) :
		     ordinary_wallet_command(b.command) && !ordinary_cash_role_current(b))
		return false;
	if (!b.constructor_present ||
	    (b.constructor.wire_version !=
		     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
	     b.constructor.wire_version !=
		     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) ||
	    !b.completed || !b.coordinator_generation || !birth_recovery_valid(b.envelope))
		return false;
	try
	{
		critical_native_recovery_envelope actual_carrier;
		if (!critical_native_mobile_birth_publication_owner::copy_context(
			    b.command, &actual_carrier) ||
		    actual_carrier.revision != b.envelope.revision ||
		    actual_carrier.phase != b.envelope.phase ||
		    actual_carrier.attachment != b.envelope.attachment ||
		    !critical_native_mobile_birth_publication_owner::observe_generation(
			    b.envelope, &b.coordinator_generation))
			return false;
		const auto uncertain = [](const auto &action)
		{ return action.started && (!action.returned || !action.succeeded); };
		if (uncertain(b.recovery.whole_binding) || uncertain(b.recovery.reference_install))
			return false;
		for (const auto &effect : b.recovery.mobile_effects)
			if (uncertain(effect))
				return false;
		for (const auto &row : b.recovery.items)
		{
			if (uncertain(row.publication) || uncertain(row.enrollment))
				return false;
			for (const auto &effect : row.effects)
				if (uncertain(effect))
					return false;
		}
		// Original receipt + current lifetime/cash/full custody proof is one
		// borrowed session, before any constructor/adoption. Rollback must be
		// confirmed before even the detached reconstruction can begin.
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		std::vector<item_ownership_runtime_entry> custody;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
				throw EIO;
			quest_mobile_native_image current;
			if ((shared ? !shared_cash_role_current(b) :
				      ordinary_wallet_command(b.command) &&
					      !ordinary_cash_role_current(b)) ||
			    lock_birth_publication(connection, b.command, b.completion, &current,
						   &custody,
						   shared ? &original_checkpoint : nullptr) ||
			    !same_image(current, b.image) ||
			    custody.size() != b.image.items.size() || !transaction.same_session())
				throw EAGAIN;
			proven = true;
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		if (b.cold_rebuilding)
		{
			quest_mobile_native_constructor_digest build, procedure, tail;
			if (!native_mobile_birth_running_artifact_digest(&build) ||
			    build != b.constructor.build_digest ||
			    !native_mobile_birth_procedure_capture(b.reference.mobile_vnum, build,
								   &procedure) ||
			    procedure != b.constructor.procedure_after ||
			    !native_mobile_birth_reset_tail_capture(b.reference.mobile_vnum,
								    b.reference.birthplace_vnum,
								    b.shop, &tail) ||
			    tail != b.constructor.reset_tail)
				return false;
			std::copy(custody.begin(), custody.end(), b.current_custody.begin());
			return resume_cold_projection(index);
		}
		// Complete current world identity comes from actual native references,
		// never prototype similarity, room occupancy or an invented runtime ID.
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> encoded;
		if (quest_mobile_native_reference_encode(b.reference, &encoded) !=
		    player_snapshot_codec_result::ok)
			return false;
		// Refuse corrupt global lists before scanning identity/UID membership.
		for (P_char slow = character_list, fast = character_list; fast && fast->next;)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
				return false;
		}
		for (P_obj slow = object_list, fast = object_list; fast && fast->next;)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
				return false;
		}
		P_char existing = nullptr;
		for (P_char mob = character_list; mob; mob = mob->next)
		{
			quest_mobile_native_reference reference;
			const auto *binding = mob->native_mobile_binding.encoded_reference_;
			if (std::all_of(binding, binding + QUEST_MOBILE_NATIVE_REFERENCE_BYTES,
					[](uint8_t value) { return value == 0; }))
				continue;
			if (quest_mobile_native_reference_decode(
				    { binding, QUEST_MOBILE_NATIVE_REFERENCE_BYTES }, &reference) !=
			    player_snapshot_codec_result::ok)
				return false;
			if (reference.mobile_instance_id != b.reference.mobile_instance_id &&
			    reference.birth_operation.bytes != b.reference.birth_operation.bytes)
				continue;
			std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> found;
			if (existing ||
			    !quest_mobile_native_reference_copy(mob, mob->runtime_id, &reference) ||
			    quest_mobile_native_reference_encode(reference, &found) !=
				    player_snapshot_codec_result::ok ||
			    found != encoded)
				return false;
			existing = mob;
		}
		const bool consumed = b.recovery.mobile_effects[0].succeeded;
		if (existing && !consumed)
			return false;
		if (existing)
		{
			const bool rolled =
				b.constructor.wire_version ==
					NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION &&
				b.constructor.alchemist_choice !=
					original_alchemist_choice::not_attempted;
			if (!alchemist_choice_compatible(existing, b.constructor) ||
			    existing->only.npc->alchemist_vial_roll_done != rolled)
				return false;
			quest_mobile_native_image observed;
			if (quest_mobile_native_capture(
				    existing, b.reference, quest_mobile_lifetime_state::live,
				    b.reference.birth_operation, 1,
				    &observed) != player_snapshot_capture_result::ok ||
			    !same_image(observed, b.image))
				return false;
			if (shared &&
			    !shared_keeper_checkpoint_current(b, existing, existing->runtime_id))
				return false;
		}
		for (const auto &item : b.stock)
		{
			P_obj found = nullptr;
			for (P_obj object = object_list; object; object = object->next)
				if (object->obj_uid == item.uid)
				{
					if (found || !existing ||
					    !b.recovery.mobile_effects[0].succeeded)
						return false;
					found = object;
				}
			if (existing && !found)
				return false;
		}
		quest_mobile_native_constructor_digest build;
		if (!native_mobile_birth_running_artifact_digest(&build) ||
		    build != b.constructor.build_digest)
			return false;
		if (existing)
		{
			quest_mobile_native_constructor_digest procedure, tail;
			if (!native_mobile_birth_procedure_capture(b.reference.mobile_vnum, build,
								   &procedure) ||
			    procedure != b.constructor.procedure_after ||
			    !native_mobile_birth_reset_tail_capture(b.reference.mobile_vnum,
								    b.reference.birthplace_vnum,
								    b.shop, &tail) ||
			    tail != b.constructor.reset_tail)
				return false;
		}
		if (b.character || b.cold_adopted)
			if (!clear_cold_projection(index))
				return false;
		if (existing)
		{
			b.cold_adopted = true;
			for (size_t row = 0; row < b.stock.size(); ++row)
			{
				auto &item = b.stock[row];
				const auto &progress = b.recovery.items[row];
				const auto literal =
					std::find_if(b.image.items.begin(), b.image.items.end(),
						     [&](const auto &value)
						     { return value.object_uid == item.uid; });
				if (literal == b.image.items.end() ||
				    !progress.publication.succeeded)
					throw EILSEQ;
				const size_t image_row =
					static_cast<size_t>(literal - b.image.items.begin());
				for (P_obj object = object_list; object; object = object->next)
					if (object->obj_uid == item.uid)
						item.object = object;
				item.stage = std::make_unique<quest_mobile_native_item_stage>();
				item.effects.resize(progress.effects.size());
				for (size_t step = 0; step < progress.effects.size(); ++step)
				{
					const auto &effect = progress.effects[step];
					item.effects[step] = { effect.started, effect.returned,
							       effect.succeeded, effect.periodic };
				}
				if (!quest_mobile_native_item_stage::adopt_published(
					    *literal, b.recipes[image_row], item.object,
					    { progress.next_step, progress.current_step_started,
					      progress.admitted, progress.published },
					    item.effects, item.stage.get()))
				{
					item.stage
						.reset(); // Strong failed adoption left the output empty.
					throw EAGAIN;
				}
				item.adopted_metadata = true;
				item.published = true;
				item.enrolled = progress.enrollment.succeeded;
				item.enrollment_started = progress.enrollment.started;
				item.enrollment_returned = progress.enrollment.returned;
			}
			if (!charge() || !b.mobile.adopt_published(existing, existing->runtime_id,
								   b.room, b.image, b.recovery))
				throw EAGAIN;
			b.character = existing;
			b.runtime_id = existing->runtime_id;
			b.mobile_started = true;
			b.mobile_consumed = true;
		}
		else
		{
			// Drain/completion observers never reconstruct a new physical actor.
			if (!allow_reconstruction)
				return false;
			if (!b.mobile.restore_constructor(b.constructor, build))
				return false;
			b.character = b.mobile.character();
			b.runtime_id = b.character->runtime_id;
			b.mobile_bytes = sizeof(*b.character) + sizeof(*b.character->only.npc);
			GET_BIRTHPLACE(b.character) = b.reference.birthplace_vnum;
			apply_zone_modifier(b.character);
			if (b.shop >= 0)
				bind_shopkeeper(b.character, b.shop);
			// These original pre-capture callback routes remain unfinished;
			// cold replay never silently drops them or treats them as pure.
			if (world[b.room].funct || (IS_SET(b.character->specials.act, ACT_SPEC) &&
						    mob_index[b.rnum].func.mob))
				throw EAGAIN;
			if (!alchemist_choice_compatible(b.character, b.constructor) ||
			    (b.constructor.wire_version ==
				     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION &&
			     !npc_alchemist_original_birth::restore_latch(
				     b.character, static_cast<native_alchemist_vial_choice>(
							  b.constructor.alchemist_choice))))
				throw EAGAIN;
			std::vector<P_obj> forest(b.image.items.size(), nullptr);
			for (auto &item : b.stock)
			{
				const auto literal =
					std::find_if(b.image.items.begin(), b.image.items.end(),
						     [&](const auto &value)
						     { return value.object_uid == item.uid; });
				if (literal == b.image.items.end())
					throw EILSEQ;
				const size_t row =
					static_cast<size_t>(literal - b.image.items.begin());
				item.stage = std::make_unique<quest_mobile_native_item_stage>();
				if (!quest_mobile_native_item_stage::restore(
					    *literal, b.recipes[row], item.stage.get()) &&
				    !quest_mobile_native_item_stage::restore_bound(
					    *literal, b.recipes[row], item.stage.get()) &&
				    !quest_mobile_native_item_stage::restore_rebind(
					    *literal, b.recipes[row], item.stage.get()))
				{
					item.stage
						.reset(); // Strong failed restore retained no object.
					throw EAGAIN;
				}
				item.object = item.stage->object();
				const auto &saved = b.recovery.items[&item - b.stock.data()];
				item.effects.resize(saved.effects.size());
				for (size_t step = 0; step < saved.effects.size(); ++step)
				{
					const auto &effect = saved.effects[step];
					item.effects[step] = { effect.started, effect.returned,
							       effect.succeeded, effect.periodic };
				}
				forest[row] = item.object;
			}
			// Final literal rows already include their final container weights.
			// This private restore links that final image exactly; it does not
			// rerun original G/E/P grouping, weight increments or enchantments.
			int64_t carrying_weight = 0;
			int64_t carrying_count = 0;
			for (const auto &literal : b.image.items)
				if (literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
				{
					carrying_weight += encumbrance_weight(literal.weight) /
							   (literal.equipment_slot ? 2 : 1);
					if (!literal.equipment_slot)
						++carrying_count;
				}
			if (carrying_weight > INT_MAX || carrying_count > INT_MAX)
				throw EILSEQ;
			for (size_t offset = forest.size(); offset; --offset)
			{
				const size_t row = offset - 1;
				P_obj object = forest[row];
				const auto &literal = b.image.items[row];
				if (literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				{
					P_obj parent =
						forest[static_cast<size_t>(literal.parent_index)];
					object->loc_p = LOC_INSIDE;
					object->loc.inside = parent;
					object->next_content = parent->contains;
					parent->contains = object;
				}
				else if (literal.equipment_slot)
				{
					const int slot = literal.equipment_slot - 1;
					b.character->equipment[slot] = object;
					object->loc_p = LOC_WORN;
					object->loc.wearing = b.character;
				}
				else
				{
					object->loc_p = LOC_CARRIED;
					object->loc.carrying = b.character;
					object->next_content = b.character->carrying;
					b.character->carrying = object;
				}
			}
			GET_CARRYING_W(b.character) = static_cast<int>(carrying_weight);
			IS_CARRYING_N(b.character) = static_cast<int>(carrying_count);
			std::vector<quest_mobile_native_item_binding> inputs;
			inputs.reserve(b.stock.size());
			for (const auto &item : b.stock)
				inputs.push_back(item.stage->binding_input());
			if (!shop_trade_original_procedure_binding_stage::prepare_native_birth(
				    inputs, b.bindings) ||
			    !b.bindings.valid() || !charge())
				throw EAGAIN;
			if (b.recovery.whole_binding.succeeded)
			{
				// Rebuild the actual process-local dispatch/chain only. The complete
				// original SQL/absence cut above authorizes this checked batch; its
				// original returned journal fact remains unchanged.
				b.bindings.commit_unchecked();
			}
			if (b.recovery.reference_install.succeeded)
			{
				std::memcpy(b.character->native_mobile_binding.encoded_reference_,
					    encoded.data(), encoded.size());
				if (!install_original_cash_metadata())
					throw EAGAIN;
			}
			quest_mobile_native_image observed;
			if (quest_mobile_native_capture(
				    b.character, b.reference, quest_mobile_lifetime_state::live,
				    b.reference.birth_operation, 1,
				    &observed) != player_snapshot_capture_result::ok ||
			    !same_image(observed, b.image) || !charge())
				throw EAGAIN;
		}
		if (b.recovery.reference_install.succeeded && !install_original_cash_metadata())
			throw EAGAIN;
		std::copy(custody.begin(), custody.end(), b.current_custody.begin());
		b.binding_committed = b.recovery.whole_binding.succeeded;
		// Cold flags do not certify current runtime projection or physical proof.
		// The original publish path must perform its complete fresh cut again.
		b.runtime_applied = false;
		b.physically_proven = false;
		if (!existing)
		{
			if (shared)
			{
				size_t prefix = 0;
				for (const auto &effect : b.recovery.mobile_effects)
				{
					if (!effect.succeeded)
						break;
					++prefix;
				}
				if (!b.mobile.prepare_shared_shopkeeper_affects(
					    b.character, b.runtime_id, b.room, b.reference,
					    original_checkpoint, prefix))
					throw EAGAIN;
				b.shared_cold_affects = true;
				b.cold_rebuilding = true;
				if (!charge())
					return false; // Pure prepared stage remains owned for budget retry.
			}
			// Retain the real projection BEFORE any actual affect/scheduler
			// service starts; a partial refusal cannot enter discard cleanup.
			b.cold_rebuilding = true;
			return resume_cold_projection(index);
		}
		b.cold = false;
		return true;
	}
	catch (...)
	{
		if (!clear_cold_projection(index))
			b.blocked = true;
		charge();
		return false;
	}
#endif
}

bool quest_mobile_native_birth_owner::publish(size_t index, bool allow_reconstruction) noexcept
{
	if (index < births.size() && births[index] && reset_holds_birth(*births[index]))
		return false;
#ifdef __NO_MYSQL__
	(void)index;
	(void)allow_reconstruction;
	return false;
#else
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	const bool shared = shared_birth(b);
	flatfile_shopkeeper_record original_checkpoint;
	// Shared cold publication uses the exact original saved-affect stage.
	// Original producer admission remains independently closed.
	if (shared && (!shared_shop_command(b.command) || !b.shared_checkpoint ||
		       !shared_cash_role_current(b) || !birth_recovery_valid(b.envelope) ||
		       !flatfile_shopkeeper_initial_checkpoint_decode(*b.shared_checkpoint,
								      &original_checkpoint)))
		return false;
	const auto install_original_cash_metadata = [&]() noexcept
	{
		if (!b.character || !b.image.cash || !b.completed ||
		    b.completion.disposition != critical_completion_disposition::execution ||
		    (b.completion.outcome != critical_apply_outcome::applied &&
		     b.completion.outcome != critical_apply_outcome::already_applied))
			return false;
		birth_wallet_receipt_values result;
		economic_frozen_intent intent;
		if (!decode_birth_wallet_receipt(b.command, b.completion, &result) ||
		    economic_intent_decode(b.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(b.command, intent) !=
			    economic_accounting_error::ok ||
		    result.mobile_instance_id != b.reference.mobile_instance_id ||
		    result.cash_revision != b.image.cash->revision ||
		    (shared ? result.wallet_mapping_id != 0 : !result.wallet_mapping_id) ||
		    critical_operation_id_is_zero(intent.admission.metadata.lineage) ||
		    critical_operation_id_is_zero(intent.admission.metadata.epoch))
			return false;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_CASH_BINDING_METADATA_BYTES> exact{},
			current{};
		const auto put = [](uint8_t *out, uint64_t value) noexcept
		{
			for (size_t i = 0; i < 8; ++i)
				out[i] = static_cast<uint8_t>(value >> (8 * i));
		};
		put(exact.data(), result.cash_revision);
		put(exact.data() + 8, result.wallet_mapping_id);
		std::copy(intent.admission.metadata.lineage.bytes.begin(),
			  intent.admission.metadata.lineage.bytes.end(), exact.begin() + 16);
		std::copy(intent.admission.metadata.epoch.bytes.begin(),
			  intent.admission.metadata.epoch.bytes.end(), exact.begin() + 32);
		auto &binding = b.character->native_mobile_binding;
		std::copy(std::begin(binding.cash_revision_), std::end(binding.cash_revision_),
			  current.begin());
		std::copy(std::begin(binding.wallet_mapping_id_),
			  std::end(binding.wallet_mapping_id_), current.begin() + 8);
		std::copy(std::begin(binding.lineage_), std::end(binding.lineage_),
			  current.begin() + 16);
		std::copy(std::begin(binding.birth_epoch_), std::end(binding.birth_epoch_),
			  current.begin() + 32);
		if (current != exact && std::any_of(current.begin(), current.end(),
						    [](uint8_t byte) { return byte != 0; }))
			return false;
		// Invoked only after this original owner's genuine SQL/image/source cut.
		// Installs observed runtime metadata; never writes a wallet or issues IDs.
		std::copy_n(exact.begin(), 8, binding.cash_revision_);
		std::copy_n(exact.begin() + 8, 8, binding.wallet_mapping_id_);
		std::copy_n(exact.begin() + 16, 16, binding.lineage_);
		std::copy_n(exact.begin() + 32, 16, binding.birth_epoch_);
		return true;
	};

	if (b.completed && b.submitted &&
	    b.completion.disposition == critical_completion_disposition::never_admitted)
	{
		if (!b.coordinator_generation)
			critical_native_mobile_birth_publication_owner::observe_generation(
				b.envelope, &b.coordinator_generation);
		if (!critical_native_mobile_birth_publication_owner::cancel_refusal(
			    b.envelope, b.completion, b.coordinator_generation, cleanup_refusal,
			    &b))
			return false;
		births[index].reset();
		charge();
		return true;
	}
	if (!settle_checkpoint(index))
		return false;
	if (!b.completed || b.blocked || !b.coordinator_generation || !b.submitted ||
	    b.completion.disposition != critical_completion_disposition::execution ||
	    (b.completion.outcome != critical_apply_outcome::applied &&
	     b.completion.outcome != critical_apply_outcome::already_applied))
		return false;
	if (b.cold && !recover_cold(index, allow_reconstruction))
		return false;
	try
	{
		if (!b.recovery.receipt_present ||
		    (b.envelope.phase == critical_native_recovery_phase::execution_pending &&
		     !same_completion(b.recovery.receipt, b.completion)))
		{
			if (b.recovery.receipt_present &&
			    !same_economic_completion(b.recovery.receipt, b.completion))
				return false;
			auto next = b.recovery;
			next.receipt_present = true;
			next.receipt = b.completion;
			if (next.stage == native_mobile_birth_recovery_stage::captured)
				next.stage = native_mobile_birth_recovery_stage::publishing;
			// Actual replay metadata may change, while the sealed economic core and
			// all original returned effects remain exact. Guarded ACK needs this
			// genuine current receipt, not a fabricated copy of its old timestamps.
			if (!checkpoint(index, next))
				return false;
		}
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
			quest_mobile_native_image current;
			std::vector<item_ownership_runtime_entry> custody;
			if ((shared ? !shared_cash_role_current(b) :
				      ordinary_wallet_command(b.command) &&
					      !ordinary_cash_role_current(b)) ||
			    lock_birth_publication(connection, b.command, b.completion, &current,
						   &custody,
						   shared ? &original_checkpoint : nullptr) ||
			    !same_image(b.image, current))
				throw EAGAIN;
			if (custody.size() != b.current_custody.size())
				throw EILSEQ;
			std::copy(custody.begin(), custody.end(), b.current_custody.begin());
			if (!b.mobile_consumed)
			{
				if (b.mobile_started || !b.character ||
				    find_character_by_runtime_id(b.runtime_id) ||
				    (!b.recovery.whole_binding.succeeded && !b.bindings.valid()))
					throw EAGAIN;
				quest_mobile_native_image before;
				if (quest_mobile_native_capture(b.character, b.reference,
								quest_mobile_lifetime_state::live,
								b.reference.birth_operation, 1,
								&before) !=
					    player_snapshot_capture_result::ok ||
				    !same_image(before, b.image))
					throw EAGAIN;
				if (shared)
				{
					flatfile_shopkeeper_record detached;
					std::vector<uint8_t> exact;
					if (b.mobile.capture_shopkeeper_checkpoint(
						    b.character, b.runtime_id, b.room, b.shop,
						    b.reference, b.constructor, b.cash_role,
						    original_checkpoint.saved_at, &detached) !=
						    player_snapshot_capture_result::ok ||
					    !flatfile_shopkeeper_initial_checkpoint_encode(
						    detached, &exact) ||
					    exact != *b.shared_checkpoint)
						throw EAGAIN;
				}
				for (const auto &item : b.stock)
					if (item.stage && !item.published)
					{
						for (P_obj o = object_list; o; o = o->next)
							if (o == item.object ||
							    o->obj_uid == item.uid)
								throw EAGAIN;
						item_ownership_runtime_entry previous{};
						if (item_ownership_runtime_lookup(item.uid,
										  &previous))
							throw EAGAIN;
					}
				std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> encoded;
				if (quest_mobile_native_reference_encode(b.reference, &encoded) !=
				    player_snapshot_codec_result::ok)
					throw EILSEQ;
				if (!b.recovery.reference_install.succeeded)
				{
					for (uint8_t value :
					     b.character->native_mobile_binding.encoded_reference_)
						if (value)
							throw EAGAIN;
				}
				else if (std::memcmp(b.character->native_mobile_binding
							     .encoded_reference_,
						     encoded.data(), encoded.size()))
					throw EAGAIN;

				int run = prepare_action(index, BINDINGS, 0, 0);
				if (run < 0)
					throw EAGAIN;
				if (run)
				{
					b.bindings.commit_unchecked();
					b.binding_committed = true;
					if (!finish_action(index, { true, true, true, false }))
						throw EAGAIN;
				}
				run = prepare_action(index, REFERENCE, 0, 0);
				if (run < 0)
					throw EAGAIN;
				if (run)
				{
					std::memcpy(b.character->native_mobile_binding
							    .encoded_reference_,
						    encoded.data(), encoded.size());
					if (!install_original_cash_metadata())
						throw EAGAIN;
					if (!finish_action(index, { true, true, true, false }))
						throw EAGAIN;
				}
				if (!install_original_cash_metadata())
					throw EAGAIN;
				for (size_t row = 0; row < b.stock.size(); ++row)
				{
					auto &item = b.stock[row];
					if (!item.stage)
						throw EILSEQ;
					// The stage's retained ownership changes only under this original cut.
					item.stage->retain_admitted();
					run = prepare_action(index, ITEM_PUBLICATION, row, 0);
					if (run < 0)
						throw EAGAIN;
					if (run)
					{
						const bool published = item.stage->publish() ==
								       item.object;
						item.published = published;
						if (!finish_action(index, { true, true, published,
									    false }))
							throw EAGAIN;
					}
				}
			}
			// Exact current receipt/native cut above is required on every retry.
			// Each private stage independently owns its once-only started latch.
			for (size_t step = 0; step < 8; ++step)
			{
				// The genuine room service and its original returned fact must
				// precede affect event rearm and every subsequent publication
				// choice. Warm stages never enter this cold-only service.
				if (b.shared_cold_affects && step >= 2 &&
				    (!b.recovery.mobile_effects[1].succeeded || !charge() ||
				     !b.mobile.finish_shared_shopkeeper_affects_after_room(
					     b.character, b.room) ||
				     !charge()))
					throw EAGAIN;
				const auto existing = b.recovery.mobile_effects[step];
				if (existing.returned && existing.succeeded)
					continue;
				const int choice_index = step == 2 ? 0 :
							 step == 4 ? 1 :
							 step == 5 ? 2 :
							 step == 6 ? 3 :
								     -1;
				if (choice_index >= 0 &&
				    !b.recovery.mobile_choices[choice_index].chosen)
				{
					if (!b.chosen_context)
					{
						b.chosen_context = std::make_unique<
							native_mobile_birth_recovery_context>(
							b.recovery);
						if (!charge())
						{
							b.chosen_context.reset();
							throw ENOMEM;
						}
						auto &choice =
							b.chosen_context
								->mobile_choices[choice_index];
						if (!b.mobile.choose_publication_step(
							    step, b.character,
							    b.recovery.mobile_effects[3], &choice))
						{
							b.chosen_context.reset();
							throw EAGAIN;
						}
					}
					// A chosen delay survives all encoding/CAS refusals; do not reroll it.
					if (!checkpoint(index, *b.chosen_context))
						throw EAGAIN;
					b.chosen_context.reset();
				}
				const int run = prepare_action(index, MOBILE_EFFECT, 0, step);
				if (run < 0)
					throw EAGAIN;
				if (!run)
					continue;
				native_mobile_birth_recovery_effect actual;
				P_char after = nullptr;
				native_mobile_birth_recovery_choice choice;
				if (choice_index >= 0)
					choice = b.recovery.mobile_choices[choice_index];
				b.mobile.publication_step(step, b.room, b.character, choice, actual,
							  &after);
				if (step == 0)
				{
					b.mobile_started = actual.started;
					b.mobile_consumed = actual.returned && actual.succeeded;
				}
				if (!finish_action(index, actual))
					throw EAGAIN;
				if (!after || after != b.character)
					throw EAGAIN;
			}
			P_char live = find_character_by_runtime_id(b.runtime_id);
			if (!live || live != b.character || !actual_objects(b))
				throw EAGAIN;
			quest_mobile_native_reference bound;
			std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> observed_reference,
				original_reference;
			if (!quest_mobile_native_reference_copy(live, b.runtime_id, &bound) ||
			    quest_mobile_native_reference_encode(bound, &observed_reference) !=
				    player_snapshot_codec_result::ok ||
			    quest_mobile_native_reference_encode(b.reference,
								 &original_reference) !=
				    player_snapshot_codec_result::ok ||
			    observed_reference != original_reference)
				throw EAGAIN;

			for (size_t row = 0; row < b.stock.size(); ++row)
			{
				auto &item = b.stock[row];
				if (!item.stage)
					continue; // Metadata may transfer only after final proof.
				int run = prepare_action(index, ITEM_ENROLLMENT, row, 0);
				if (run < 0)
					throw EAGAIN;
				if (run)
				{
					item.enrollment_started = true;
					const bool enrolled =
						quest_mobile_native_local_stock::enroll(item.object,
											live);
					item.enrollment_returned = true;
					item.enrolled = enrolled;
					if (!finish_action(index, { true, true, enrolled, false }))
						throw EAGAIN;
				}
				for (size_t step = 0; step < item.effects.size(); ++step)
				{
					run = prepare_action(index, ITEM_EFFECT, row, step);
					if (run < 0)
						throw EAGAIN;
					if (!run)
						continue;
					auto &effect = item.effects[step];
					item.stage->publication_step(step, item.object, effect);
					native_mobile_birth_recovery_effect actual{
						effect.started, effect.returned, effect.succeeded,
						effect.periodic
					};
					if (!finish_action(index, actual))
						throw EAGAIN;
					live = find_character_by_runtime_id(b.runtime_id);
					if (!live || live != b.character || !actual_objects(b))
						throw EAGAIN;
				}
			}
			if (shared)
			{
				// Reauthenticate the full SQL/cache cut on every retry, including
				// genuine owner absence or present-zero for an empty stock.
				if (!shop_trade_current_runtime_owner::publish_native_birth(
					    connection, b.command, original_checkpoint,
					    b.completion))
					throw EAGAIN;
				b.runtime_applied = true;
			}
			if (!b.runtime_applied)
			{
				if (!item_ownership_runtime_hydrate_many_atomic(
					    b.current_custody.data(), b.current_custody.size()) ||
				    !item_ownership_runtime_hydrate_owner(
					    { item_owner_type::native_mobile,
					      b.reference.mobile_instance_id, 0 },
					    1))
				{
					// Cache projection can refuse transient allocation. Retain the real
					// published graph and retry only after the full original SQL/world cut.
					throw EAGAIN;
				}
				b.runtime_applied = true;
			}
			uint64_t observed_owner_revision = 0;
			if (!shared && (!item_ownership_runtime_peek_owner_revision(
						{ item_owner_type::native_mobile,
						  b.reference.mobile_instance_id, 0 },
						&observed_owner_revision) ||
					observed_owner_revision != 1))
				throw EAGAIN;
			for (const auto &expected : b.current_custody)
			{
				item_ownership_runtime_entry actual{};
				if (!item_ownership_runtime_lookup(expected.item_uid, &actual) ||
				    actual.item_uid != expected.item_uid ||
				    actual.root_item_uid != expected.root_item_uid ||
				    actual.parent_item_uid != expected.parent_item_uid ||
				    !item_owner_identity_equal(actual.owner, expected.owner) ||
				    actual.item_revision != expected.item_revision ||
				    actual.owner_revision != expected.owner_revision ||
				    actual.vnum != expected.vnum || actual.state != expected.state)
					throw EAGAIN;
			}
			quest_mobile_native_image after;
			if (quest_mobile_native_capture(live, b.reference,
							quest_mobile_lifetime_state::live,
							b.reference.birth_operation, 1, &after) !=
				    player_snapshot_capture_result::ok ||
			    !same_image(after, b.image))
				throw EAGAIN;
			if (shared)
			{
				// Observe the actual indexed lifetime, real room/SHOP binding,
				// saved-affect multiset, denominations and complete literal forest.
				// saved_at is the authenticated original stamp (there is no live
				// actor save clock); the locked SQL row independently proves it.
				if (find_character_by_runtime_id(b.runtime_id) != live ||
				    live != b.character || live->runtime_id != b.runtime_id ||
				    !IS_NPC(live) || !live->only.npc || !IS_ALIVE(live) ||
				    live->in_room != b.room || b.shop < 0 ||
				    live->only.npc->shopkeeper_shop_id != b.shop ||
				    original_checkpoint.shop_id != static_cast<uint32_t>(b.shop))
					throw EAGAIN;
				for (P_char slow = character_list, fast = character_list;
				     fast && fast->next;)
				{
					slow = slow->next;
					fast = fast->next->next;
					if (slow == fast)
						throw EAGAIN;
				}
				size_t members = 0;
				for (P_char mob = character_list; mob; mob = mob->next)
				{
					if (mob == live)
						++members;
					const auto *bytes =
						mob->native_mobile_binding.encoded_reference_;
					if (std::all_of(bytes,
							bytes + QUEST_MOBILE_NATIVE_REFERENCE_BYTES,
							[](uint8_t value) { return value == 0; }))
						continue;
					quest_mobile_native_reference candidate;
					if (quest_mobile_native_reference_decode(
						    { bytes, QUEST_MOBILE_NATIVE_REFERENCE_BYTES },
						    &candidate) != player_snapshot_codec_result::ok)
						throw EAGAIN;
					if ((candidate.mobile_instance_id ==
						     b.reference.mobile_instance_id ||
					     candidate.birth_operation.bytes ==
						     b.reference.birth_operation.bytes) &&
					    mob != live)
						throw EAGAIN;
				}
				if (members != 1)
					throw EAGAIN;
				P_char people = world[b.room].people;
				for (P_char slow = people, fast = people;
				     fast && fast->next_in_room;)
				{
					slow = slow->next_in_room;
					fast = fast->next_in_room->next_in_room;
					if (slow == fast)
						throw EAGAIN;
				}
				members = 0;
				for (P_char mob = people; mob; mob = mob->next_in_room)
					if (mob == live)
						++members;
				flatfile_shopkeeper_record observed;
				std::vector<uint8_t> exact;
				if (members != 1 ||
				    flatfile_shopkeeper_capture(live, original_checkpoint.shop_id,
								1, original_checkpoint.saved_at,
								&observed) !=
					    player_snapshot_capture_result::ok ||
				    quest_mobile_native_items_observe(live, b.reference,
								      &observed.items) !=
					    player_snapshot_capture_result::ok ||
				    !flatfile_shopkeeper_initial_checkpoint_encode(observed,
										   &exact) ||
				    exact != *b.shared_checkpoint ||
				    !install_original_cash_metadata())
					throw EAGAIN;
			}
			quest_mobile_native_image final;
			std::vector<item_ownership_runtime_entry> final_custody;
			if ((shared ? !shared_cash_role_current(b) :
				      ordinary_wallet_command(b.command) &&
					      !ordinary_cash_role_current(b)) ||
			    lock_birth_publication(connection, b.command, b.completion, &final,
						   &final_custody,
						   shared ? &original_checkpoint : nullptr) ||
			    !same_image(final, b.image) || !transaction.same_session())
				throw EAGAIN;
			proven = true;
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		b.physically_proven = true;
		if (b.recovery.stage != native_mobile_birth_recovery_stage::physically_proven)
		{
			auto next = b.recovery;
			next.runtime_applied = b.runtime_applied;
			next.stage = native_mobile_birth_recovery_stage::physically_proven;
			if (!checkpoint(index, next))
				return false;
		}
		// All retained factory steps really returned successfully before ownership
		// of their metadata transfers away. The world body remains observed by UID.
		for (auto &item : b.stock)
			if (item.stage)
			{
				if (!item.stage->release_published())
				{
					b.blocked = true;
					return false;
				}
				item.stage.reset();
			}
		if (b.envelope.phase == critical_native_recovery_phase::execution_pending)
		{
			if (!b.ack_successor)
			{
				if (b.envelope.revision == UINT64_MAX)
					return false;
				b.ack_successor =
					std::make_unique<critical_native_recovery_envelope>(
						b.envelope);
				++b.ack_successor->revision;
				b.ack_successor->phase =
					critical_native_recovery_phase::continuation_pending;
				if (!birth_recovery_successor(b.envelope, *b.ack_successor) ||
				    !charge())
				{
					b.ack_successor.reset();
					return false;
				}
			}
			if (!critical_native_mobile_birth_publication_owner::acknowledge(
				    b.envelope, b.completion, b.coordinator_generation))
				return false;
			// Guarded ACK authenticated the genuine CURRENT receipt and performed this
			// exact preallocated phase transition. No copy/encode allocation follows it.
			b.envelope = std::move(*b.ack_successor);
			b.ack_successor.reset();
		}
		if (b.command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION ||
		    ordinary_wallet_command(b.command))
		{
			if (!critical_native_mobile_birth_publication_owner::retire(
				    b.envelope, b.coordinator_generation,
				    retain_published_constructor_origin, nullptr))
				return false;
		}
		else if (!critical_native_mobile_birth_publication_owner::retire(b.envelope))
			return false;
		births[index].reset();
		charge();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_mobile_native_birth_owner::restore(const critical_command &command) noexcept
{
	return restore_command(command, nullptr);
}
bool quest_mobile_native_birth_owner::restore_command(
	const critical_command &command, const std::vector<uint8_t> *shared_checkpoint) noexcept
{
	if (command.type != critical_command_type::native_mobile_birth)
		return true;
	try
	{
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		quest_mobile_native_constructor_recipe constructor;
		const bool shared = shared_shop_command(command);
		if (shared && (!shared_checkpoint || shared_checkpoint->empty()))
			return false;
		const bool ordinary = ordinary_wallet_command(command);
		const bool constructor_present =
			ordinary ||
			command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION;
		native_mobile_birth_cash_role_recipe cash_role;
		std::vector<uint8_t> canonical;
		const auto decoded =
			ordinary ?
				native_mobile_birth_cash_role_command_decode(command, &image,
									     &recipes, &cash_role) :
			constructor_present ?
				native_mobile_birth_command_decode(command, &image, &recipes,
								   &constructor) :
				native_mobile_birth_command_decode(command, &image, &recipes);
		if (ordinary)
			constructor = cash_role.original;
		if (decoded != economic_accounting_error::ok ||
		    (ordinary &&
		     cash_role.role != (shared ? native_mobile_birth_cash_role::shared_shopkeeper :
						 native_mobile_birth_cash_role::ordinary_wallet)) ||
		    critical_command_encode(command, &canonical) !=
			    critical_command_codec_result::ok)
			return false;
		for (const auto &b : births)
			if (b && b->reference.birth_operation.bytes == command.operation_id.bytes)
				return b->canonical == canonical && same_image(b->image, image) &&
				       (!shared || (b->shared_checkpoint &&
						    *b->shared_checkpoint == *shared_checkpoint));
		for (const auto &previous : births)
			if (previous)
			{
				const auto &left = previous->reference.birth_source;
				const auto &right = image.reference.birth_source;
				if (previous->reference.mobile_instance_id ==
					    image.reference.mobile_instance_id ||
				    (left.kind == right.kind &&
				     left.source.bytes == right.source.bytes &&
				     left.generation.bytes == right.generation.bytes &&
				     left.sequence == right.sequence && left.slot == right.slot))
					return false;
				for (const auto &row : image.items)
					for (const auto &old : previous->image.items)
						if (row.object_uid == old.object_uid)
							return false;
			}
		size_t available = 0;
		while (available < births.size() && births[available])
			++available;
		if (available == births.size() &&
		    births.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS)
			return false;
		auto b = std::make_unique<original_birth>();
		b->reference = image.reference;
		b->image = std::move(image);
		b->recipes = std::move(recipes);
		b->constructor = constructor;
		b->constructor_present = constructor_present;
		b->cash_role = cash_role;
		b->cash_role_present = ordinary;
		if (shared)
			b->shared_checkpoint =
				std::make_unique<const std::vector<uint8_t>>(*shared_checkpoint);
		if (constructor_present &&
		    (constructor.wire_version ==
			     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
		     constructor.wire_version ==
			     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION))
		{
			// Retained reset inputs must describe this exact original native birth.
			// The factory later authenticates the current selected room/shop witness.
			if (constructor.reset_room_vnum != b->reference.birthplace_vnum)
				return false;
			b->shop = constructor.reset_shop_index;
		}
		b->command = command;
		b->canonical = std::move(canonical);
		b->rnum = real_mobile(b->reference.mobile_vnum);
		b->zone = real_zone(b->reference.reset_zone_vnum);
		b->room = real_room0(b->reference.birthplace_vnum);
		if (b->rnum < 0 || b->zone < 0 || b->room < 0)
			return false;
		b->cold = true;
		b->cold_replay_enrolled = true;
		b->sealed = true;
		b->submitted = true;
		b->current_custody.resize(b->image.items.size());
		for (const auto &row : b->image.items)
		{
			original_item item;
			item.uid = row.object_uid;
			item.rnum = real_object(row.vnum);
			if (item.rnum < 0)
				return false;
			b->stock.push_back(std::move(item));
		}
		if (available == births.size())
			births.push_back(std::move(b));
		else
			births[available] = std::move(b);
		// Passive values only; no constructor, RNG, IDs, SQL or coordinator reentry.
		// Image-only legacy records have no original constructor/event recipe.
		// Only typed NBC2/NBC3 routes can enroll actual cold publication.
		if (!quest_mobile_native_birth_owner::charge())
		{
			// Passive registration has performed no native or persistence effects.
			// Refuse without retaining an uncharged new replay body.
			births[available].reset();
			charge();
			return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_native_birth_owner::restore(
	const critical_native_recovery_envelope &envelope) noexcept
{
	// Called passively under coordinator_mutex. No coordinator reentry, SQL,
	// constructor/RNG/UID allocation or physical publication occurs here.
	if (envelope.command.type != critical_command_type::native_mobile_birth ||
	    !birth_recovery_valid(envelope))
		return false;
	try
	{
		native_mobile_birth_recovery_context context;
		std::vector<uint8_t> checkpoint;
		const bool shared = shared_shop_command(envelope.command);
		if (birth_recovery_decode(envelope.command, envelope.attachment, &context,
					  &checkpoint) != economic_accounting_error::ok)
			return false;
		critical_native_recovery_envelope retained = envelope;
		for (const auto &entry : births)
			if (entry && entry->reference.birth_operation.bytes ==
					     envelope.command.operation_id.bytes)
				return (!shared || (entry->shared_checkpoint &&
						    *entry->shared_checkpoint == checkpoint)) &&
				       entry->envelope.revision == envelope.revision &&
				       entry->envelope.phase == envelope.phase &&
				       entry->envelope.attachment == envelope.attachment &&
				       entry->canonical == [&]()
				{
					std::vector<uint8_t> bytes;
					if (critical_command_encode(envelope.command, &bytes) !=
					    critical_command_codec_result::ok)
						throw EILSEQ;
					return bytes;
				}();
		quest_mobile_native_image image;
		if (ordinary_wallet_command(envelope.command))
		{
			std::vector<native_mobile_birth_item_recipe> recipes;
			native_mobile_birth_cash_role_recipe role;
			if (native_mobile_birth_cash_role_command_decode(envelope.command, &image,
									 &recipes, &role) !=
				    economic_accounting_error::ok ||
			    role.role != (shared ?
						  native_mobile_birth_cash_role::shared_shopkeeper :
						  native_mobile_birth_cash_role::ordinary_wallet))
				return false;
		}
		else if (native_mobile_birth_command_decode(envelope.command, &image) !=
			 economic_accounting_error::ok)
			return false;
		std::vector<original_item> stock;
		stock.reserve(context.items.size());
		for (const auto &row : context.items)
		{
			const auto found = std::find_if(
				image.items.begin(), image.items.end(), [&](const auto &item)
				{ return item.object_uid == row.object_uid; });
			if (found == image.items.end())
				return false;
			original_item item;
			item.uid = found->object_uid;
			item.rnum = real_object(found->vnum);
			if (item.rnum < 0)
				return false;
			stock.push_back(std::move(item));
		}
		if (!restore_command(envelope.command, shared ? &checkpoint : nullptr))
			return false;
		for (auto &entry : births)
			if (entry && entry->reference.birth_operation.bytes ==
					     envelope.command.operation_id.bytes)
			{
				entry->stock = std::move(stock);
				entry->envelope = std::move(retained);
				entry->recovery = std::move(context);
				if (!charge())
				{
					entry.reset();
					charge();
					return false;
				}
				// Exact values remain passive. Actual cold preparation/adoption requires
				// genuine carrier generation, receipt and current SQL/world proof.
				return true;
			}
		return false;
	}
	catch (...)
	{
		return false;
	}
}
bool quest_mobile_native_birth_restore(const critical_native_recovery_envelope &envelope) noexcept
{
	return quest_mobile_native_birth_owner::restore(envelope);
}

void quest_mobile_native_birth_replay_ready(bool ready) noexcept
{
	if (nevent_is_game_thread())
		replay_ready = ready;
}
void quest_mobile_native_birth_owner::completions(const critical_completion *incoming,
						  size_t count) noexcept
{
	if (!nevent_is_game_thread() || (count && !incoming))
		return;
	for (size_t i = 0; i < count; ++i)
		for (size_t j = 0; j < births.size(); ++j)
			if (births[j] && births[j]->reference.birth_operation.bytes ==
						 incoming[i].operation_id.bytes)
			{
				auto &b = *births[j];
				if (!critical_completion_disposition_valid(incoming[i]))
				{
					b.blocked = true;
					continue;
				}
				if (b.completed && !same_completion(b.completion, incoming[i]) &&
				    !same_economic_completion(b.completion, incoming[i]))
				{
					b.blocked = true;
					continue;
				}
				b.completion = incoming[i];
				b.completed = true;
				quest_mobile_native_birth_owner::publish(j);
			}
	// Retained publication/ACK retry only. Shutdown drain must not construct/issue.
	quest_mobile_native_birth_pulse(false);
}
void quest_mobile_native_birth_owner::pulse(bool prepare_original_resets) noexcept
{
	pulse_policy(prepare_original_resets, false);
}
bool quest_mobile_native_birth_owner::recovery_pulse() noexcept
{
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active() || !replay_ready ||
	    reset_in_progress)
		return false;
	pulse_policy(false, true);
	for (const auto &b : births)
		if (b && b->cold_replay_enrolled)
			return false;
	return !deferred_overflow;
}
void quest_mobile_native_birth_owner::pulse_policy(bool prepare_original_resets,
						   bool recovery_only) noexcept
{
	if (!nevent_is_game_thread())
		return;
	try
	{
		for (size_t i = 0; i < births.size(); ++i)
			if (births[i] && (!recovery_only || births[i]->cold_replay_enrolled))
			{
				auto &b = *births[i];
				if (reset_holds_birth(b))
					continue;
				if (shared_birth(b))
				{
					if (!recovery_only && prepare_original_resets &&
					    replay_ready &&
					    !economic_gameplay_authority::active_sql_recovery())
						prepare_shared_capture(i);
					// General shared admission remains CLOSED, including passive
					// cold replay: no submit, SQL, world publication or retirement.
					continue;
				}
				if (!b.cold && b.sealed && !b.blocked && !b.submitted)
				{
					if (recovery_only || !prepare_original_resets ||
					    economic_gameplay_authority::active_sql_recovery() ||
					    !replay_ready)
						continue;
					if (b.canonical.empty())
					{
						if (!b.constructor_present ||
						    !ordinary_cash_role_current(b))
						{
							b.blocked = true;
							continue;
						}
						critical_command original;
						if (economic_gameplay_authority::
							    prepare_native_mobile_birth_ordinary_wallet(
								    b.image, b.recipes, b.cash_role,
								    critical_source_site::zone_event,
								    b.accepted_usec, &original) !=
						    economic_accounting_error::ok)
							continue;
						std::vector<uint8_t> canonical;
						if (critical_command_encode(original, &canonical) !=
						    critical_command_codec_result::ok)
							continue;
						b.command = std::move(original);
						b.canonical = std::move(canonical);
						if (!quest_mobile_native_birth_owner::charge())
						{
							b.blocked = true;
							discard(i);
							continue;
						}
					}

					if (b.envelope.attachment.empty())
					{
						native_mobile_birth_recovery_context context;
						context.items.reserve(b.stock.size());
						for (const auto &item : b.stock)
						{
							native_mobile_birth_recovery_item row;
							row.object_uid = item.uid;
							row.effects.resize(item.effects.size());
							context.items.push_back(std::move(row));
						}
						critical_native_recovery_envelope envelope;
						envelope.command = b.command;
						envelope.revision = 1;
						envelope.phase = critical_native_recovery_phase::
							execution_pending;
						if (birth_recovery_encode(
							    b.command, context,
							    &envelope.attachment,
							    b.shared_checkpoint.get()) !=
							    economic_accounting_error::ok ||
						    !birth_recovery_initial(envelope))
							continue;
						b.envelope = std::move(envelope);
						b.recovery = std::move(context);
						if (!charge())
						{
							b.envelope = {};
							b.recovery = {};
							continue;
						}
					}
					if (ordinary_wallet_command(b.command) &&
					    !ordinary_cash_role_current(b))
						continue;
					const auto result =
						critical_native_mobile_birth_publication_owner::
							submit(b.envelope);
					if (critical_submit_result_keeps_operation(result))
						b.submitted = true;
					else if (result == critical_submit_result::invalid ||
						 result ==
							 critical_submit_result::identity_conflict)
						b.blocked = true;
				}
				if (b.submitted && !b.coordinator_generation)
					critical_native_mobile_birth_publication_owner::
						observe_generation(b.envelope,
								   &b.coordinator_generation);
				critical_completion completion{};
				if (b.submitted && !b.completed &&
				    critical_command_coordinator_get_completed(
					    b.command.operation_id, &completion))
				{
					if (!critical_completion_disposition_valid(completion))
					{
						b.blocked = true;
						continue;
					}
					b.completion = completion;
					b.completed = true;
				}
				if (b.cold && !b.completed &&
				    b.envelope.phase ==
					    critical_native_recovery_phase::continuation_pending &&
				    birth_recovery_terminal(b.envelope))
				{
					// Phase2 has no execution queue. Its original full durable receipt
					// still requires historical/current SQL proof before domain adoption.
					b.completion = b.recovery.receipt;
					b.completed = true;
				}
				if (b.completed)
					quest_mobile_native_birth_owner::publish(
						i, (prepare_original_resets || recovery_only) &&
							   replay_ready);
			}
		if (!recovery_only && prepare_original_resets && replay_ready &&
		    !economic_gameplay_authority::active_sql_recovery() &&
		    !reset_dispatch.resume_dispatch && reset_resume_current() &&
		    reset_objects_current() && charge())
		{
			reset_dispatch.resume_dispatch = true;
			reset_zone(reset_zone_rnum, reset_dispatch.force);
			// A refused front door keeps the genuine cursor and lifetime holders.
			reset_dispatch.resume_dispatch = false;
			return;
		}
		if (recovery_only || !prepare_original_resets || !replay_ready ||
		    economic_gameplay_authority::active_sql_recovery() || reset_in_progress ||
		    !deferred.size())
			return;
		for (size_t i = 0; i < deferred.size(); ++i)
		{
			const auto request = deferred[i];
			bool held = zone_table && request.zone >= 0 &&
				    request.zone <= top_of_zone_table &&
				    zone_reset_room_item_warm_pending_vnum(
					    zone_table[request.zone].number);
			for (const auto &b : births)
				if (b && b->zone == request.zone)
					held = true;
			if (held)
				continue;
			deferred.erase(deferred.begin() + i);
			reset_zone(request.zone, request.force);
			quest_mobile_native_birth_owner::charge();
			break;
		}
	}
	catch (...)
	{
		deferred_overflow = true;
	}
}
bool quest_mobile_native_birth_lifecycle_ready() noexcept
{
	if (!economic_gameplay_authority::active())
		return true;
	if (reset_in_progress || deferred_overflow || !deferred.empty())
		return false;
	for (const auto &b : births)
		if (b && !b->submitted)
			return false;
	return true;
}
bool quest_mobile_native_birth_restore(const critical_command &command) noexcept
{
	return quest_mobile_native_birth_owner::restore(command);
}
void quest_mobile_native_birth_completions(const critical_completion *incoming,
					   size_t count) noexcept
{
	quest_mobile_native_birth_owner::completions(incoming, count);
}
void quest_mobile_native_birth_pulse(bool prepare_original_resets) noexcept
{
	quest_mobile_native_birth_owner::pulse(prepare_original_resets);
}
bool quest_mobile_native_birth_recovery_pulse() noexcept
{
	return quest_mobile_native_birth_owner::recovery_pulse();
}
