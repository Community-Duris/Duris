#include "economy/native_mobile_birth_recovery.h"
#include "world/quest_mobile_native_birth.h"
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
extern void apply_zone_modifier(P_char);

namespace
{

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
			    quest_mobile_native_origin_sql_retain_locked(connection, original) ||
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
	bool alchemist_started = false, alchemist_returned = false;
	critical_command command{};
	std::vector<uint8_t> canonical;
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
	// Passive original replay enrollment survives cold reconstruction until retire.
	// This RAM-only correlation never grants SQL or coordinator authority.
	bool cold_replay_enrolled = false;
	bool completed = false, binding_committed = false, mobile_started = false;
	bool mobile_consumed = false, runtime_applied = false, physically_proven = false;
	bool retired = false;
	bool refusal_cleanup_started = false, refusal_cleanup_returned = false;
};
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
				if (!charge_envelope(bytes, b.envelope) ||
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
	for (const auto &b : births)
		if (b && !b->retired)
			for (const auto &item : b->stock)
				if (!item.published && item.rnum == rnum)
					++count;
	return count;
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
		// Retain genuine bounded/deduplicated reset requests while selected
		// recovery keeps accounting policy active and fresh admission closed.
		bool held = economic_gameplay_authority::active_sql_recovery() || !replay_ready ||
			    reset_in_progress;
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
		if (zone < 0 || zone > top_of_zone_table)
			return false;
		reset_in_progress = true;
		reset_zone_rnum = zone;
		reset_invocation = {};
		current_birth = SIZE_MAX;
		return true;
	}
	catch (...)
	{
		deferred_overflow = true;
		return false;
	}
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
		if (critical_operation_id_is_zero(reset_invocation) &&
		    !critical_operation_id_generate(&reset_invocation))
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
		return true;
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
P_obj quest_mobile_native_birth_owner::original_object(int rnum) noexcept
{
	// A reused vector hole is not original construction chronology. The current
	// original forest owns the newest constructors for supported same-forest P.
	if (current_birth < births.size() && births[current_birth])
		for (auto item = births[current_birth]->stock.rbegin();
		     item != births[current_birth]->stock.rend(); ++item)
			if (item->object && !item->published && item->rnum == rnum)
				return item->object;
	// read_object prepends each actual constructor to object_list before the
	// original P lookup, including the newly constructed object itself. Pending
	// original constructors retain that precise virtual prepend order.
	for (auto bi = births.rbegin(); bi != births.rend(); ++bi)
		if (*bi && !(*bi)->retired)
			for (auto i = (*bi)->stock.rbegin(); i != (*bi)->stock.rend(); ++i)
				if (i->object && !i->published && i->rnum == rnum)
					return i->object;
	return get_obj_num(rnum);
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
void quest_mobile_native_birth_owner::finish_reset() noexcept
{
	if (!reset_in_progress)
		return;
	seal_mobile();
	reset_in_progress = false;
	reset_zone_rnum = -1;
	reset_invocation = {};
}
bool quest_mobile_native_birth_owner::discard(size_t index) noexcept
{
	if (index >= births.size() || !births[index])
		return false;
	auto &b = *births[index];
	if (b.submitted || b.mobile_started || b.cold)
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
		if (!b.completed || !same_completion(b.completion, completion) ||
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
		if (native_mobile_birth_recovery_encode(b.command, next,
							&writer->successor.attachment) !=
			    economic_accounting_error::ok ||
		    !native_mobile_birth_recovery_successor(writer->expected, writer->successor))
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
		if (native_mobile_birth_recovery_encode(b.command, started,
							&intent->successor.attachment) !=
			    economic_accounting_error::ok ||
		    !native_mobile_birth_recovery_successor(intent->expected, intent->successor))
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
			if (native_mobile_birth_recovery_encode(b.command, writer->context,
								&writer->successor.attachment) ==
				    economic_accounting_error::ok &&
			    native_mobile_birth_recovery_successor(writer->expected,
								   writer->successor))
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
	const auto install_original_cash_metadata = [&]() noexcept
	{
		if (!b.character || !b.image.cash || !b.completed ||
		    b.completion.disposition != critical_completion_disposition::execution ||
		    (b.completion.outcome != critical_apply_outcome::applied &&
		     b.completion.outcome != critical_apply_outcome::already_applied))
			return false;
		native_mobile_birth_result result;
		economic_frozen_intent intent;
		if (!native_mobile_birth_result_decode({ b.completion.result_payload.data(),
							 b.completion.result_size },
						       &result) ||
		    economic_intent_decode(b.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(b.command, intent) !=
			    economic_accounting_error::ok ||
		    result.mobile_instance_id != b.reference.mobile_instance_id ||
		    result.cash_revision != b.image.cash->revision || !result.wallet_mapping_id ||
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
	if (!b.constructor_present ||
	    (b.constructor.wire_version !=
		     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
	     b.constructor.wire_version !=
		     NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) ||
	    !b.completed || !b.coordinator_generation ||
	    !native_mobile_birth_recovery_valid(b.envelope))
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
			if (economic_sql_native_mobile_birth_lock_publication(
				    connection, b.command, b.completion, &current, &custody) ||
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
#ifdef __NO_MYSQL__
	(void)index;
	(void)allow_reconstruction;
	return false;
#else
	if (index >= births.size() || !births[index] || !nevent_is_game_thread())
		return false;
	auto &b = *births[index];
	const auto install_original_cash_metadata = [&]() noexcept
	{
		if (!b.character || !b.image.cash || !b.completed ||
		    b.completion.disposition != critical_completion_disposition::execution ||
		    (b.completion.outcome != critical_apply_outcome::applied &&
		     b.completion.outcome != critical_apply_outcome::already_applied))
			return false;
		native_mobile_birth_result result;
		economic_frozen_intent intent;
		if (!native_mobile_birth_result_decode({ b.completion.result_payload.data(),
							 b.completion.result_size },
						       &result) ||
		    economic_intent_decode(b.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(b.command, intent) !=
			    economic_accounting_error::ok ||
		    result.mobile_instance_id != b.reference.mobile_instance_id ||
		    result.cash_revision != b.image.cash->revision || !result.wallet_mapping_id ||
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
			if (economic_sql_native_mobile_birth_lock_publication(
				    connection, b.command, b.completion, &current, &custody) ||
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
			if (!item_ownership_runtime_peek_owner_revision(
				    { item_owner_type::native_mobile,
				      b.reference.mobile_instance_id, 0 },
				    &observed_owner_revision) ||
			    observed_owner_revision != 1)
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
			quest_mobile_native_image final;
			std::vector<item_ownership_runtime_entry> final_custody;
			if (economic_sql_native_mobile_birth_lock_publication(
				    connection, b.command, b.completion, &final, &final_custody) ||
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
				if (!native_mobile_birth_recovery_successor(b.envelope,
									    *b.ack_successor) ||
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
		if (b.command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION)
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
	if (command.type != critical_command_type::native_mobile_birth)
		return true;
	try
	{
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		quest_mobile_native_constructor_recipe constructor;
		const bool constructor_present = command.payload_version ==
						 NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION;
		std::vector<uint8_t> canonical;
		const auto decoded =
			constructor_present ?
				native_mobile_birth_command_decode(command, &image, &recipes,
								   &constructor) :
				native_mobile_birth_command_decode(command, &image, &recipes);
		if (decoded != economic_accounting_error::ok ||
		    critical_command_encode(command, &canonical) !=
			    critical_command_codec_result::ok)
			return false;
		for (const auto &b : births)
			if (b && b->reference.birth_operation.bytes == command.operation_id.bytes)
				return b->canonical == canonical && same_image(b->image, image);
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
	    !native_mobile_birth_recovery_valid(envelope))
		return false;
	try
	{
		native_mobile_birth_recovery_context context;
		if (native_mobile_birth_recovery_decode(envelope.command, envelope.attachment,
							&context) != economic_accounting_error::ok)
			return false;
		critical_native_recovery_envelope retained = envelope;
		for (const auto &entry : births)
			if (entry && entry->reference.birth_operation.bytes ==
					     envelope.command.operation_id.bytes)
				return entry->envelope.revision == envelope.revision &&
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
		if (native_mobile_birth_command_decode(envelope.command, &image) !=
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
		if (!restore(envelope.command))
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
				if (!b.cold && b.sealed && !b.blocked && !b.submitted)
				{
					if (recovery_only || !prepare_original_resets ||
					    economic_gameplay_authority::active_sql_recovery() ||
					    !replay_ready)
						continue;
					if (b.canonical.empty())
					{
						if (!b.constructor_present)
						{
							b.blocked = true;
							continue;
						}
						critical_command original;
						if (economic_gameplay_authority::
							    prepare_native_mobile_birth(
								    b.image, b.recipes,
								    b.constructor,
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
						if (native_mobile_birth_recovery_encode(
							    b.command, context,
							    &envelope.attachment) !=
							    economic_accounting_error::ok ||
						    !native_mobile_birth_recovery_initial(envelope))
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
				    native_mobile_birth_recovery_terminal(b.envelope))
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
		if (recovery_only || !prepare_original_resets || !replay_ready ||
		    economic_gameplay_authority::active_sql_recovery() || reset_in_progress ||
		    !deferred.size())
			return;
		for (size_t i = 0; i < deferred.size(); ++i)
		{
			const auto request = deferred[i];
			bool held = false;
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
