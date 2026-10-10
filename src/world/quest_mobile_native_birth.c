#include "economy/native_mobile_birth_recovery.h"
#include "world/quest_mobile_native_birth.h"
#include "world/zone_reset_item_owner.h"
#include "world/db.h"
#include "persistence/persistence_mode.h"
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
#include "economy/shop.h"
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
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <type_traits>

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

struct quest_mobile_native_birth_ordinary_source_pin::implementation
{
	const void *producer = nullptr;
	std::string selected_root;
	critical_operation_id invocation{}, birth{}, lineage{}, epoch{};
	uint64_t instance = 0, runtime = 0;
	uint32_t slot = 0;
	int zone = -1, room = -1, rnum = -1;
	std::array<int32_t, 4> original_m_args{};
};
quest_mobile_native_birth_ordinary_source_pin::quest_mobile_native_birth_ordinary_source_pin(
	std::unique_ptr<implementation> state)
	: state_(std::move(state))
{
}
quest_mobile_native_birth_ordinary_source_pin::~quest_mobile_native_birth_ordinary_source_pin() =
	default;

namespace
{
std::mutex ordinary_birth_request_mutex;
std::condition_variable ordinary_birth_request_changed;
quest_mobile_native_birth_ordinary_execution_lease *ordinary_birth_request_head = nullptr;
}

bool quest_mobile_native_birth_ordinary_execution_lease::request(
	const critical_ordinary_native_execution_owner &worker) noexcept
{
	try
	{
		if (nevent_is_game_thread() || !worker.current())
			return false;
		std::unique_lock<std::mutex> lock(ordinary_birth_request_mutex);
		if (phase_ != phase::idle)
			return false;
		worker_ = &worker;
		generation_ = worker.generation();
		attempt_ = worker.attempt();
		phase_ = phase::requested;
		next_ = ordinary_birth_request_head;
		ordinary_birth_request_head = this;
		ordinary_birth_request_changed.notify_all();
		lock.unlock();
		// Registration races shutdown cancellation: recheck the real worker
		// outside request mutex so a request registered after cancel cannot wait.
		if (!worker.current())
			return false;
		lock.lock();
		ordinary_birth_request_changed.wait(
			lock,
			[&]() { return phase_ == phase::granted || phase_ == phase::refused; });
		return phase_ == phase::granted;
	}
	catch (...)
	{
		return false;
	}
}
quest_mobile_native_birth_ordinary_execution_lease::
	~quest_mobile_native_birth_ordinary_execution_lease() noexcept
{
	try
	{
		std::unique_lock<std::mutex> lock(ordinary_birth_request_mutex);
		phase_ = phase::released;
		ordinary_birth_request_changed.notify_all();
		ordinary_birth_request_changed.wait(lock, [&]() { return !game_holds_request_; });
		auto **link = &ordinary_birth_request_head;
		while (*link && *link != this)
			link = &(*link)->next_;
		if (*link == this)
			*link = next_;
		phase_ = phase::released;
		ordinary_birth_request_changed.notify_all();
	}
	catch (...)
	{
		// The genuine game-thread wait must never outlive this worker stack.
		std::terminate();
	}
}
bool quest_mobile_native_birth_ordinary_execution_lease::current() const noexcept
{
	try
	{
		const critical_ordinary_native_execution_owner *worker = nullptr;
		{
			std::lock_guard<std::mutex> lock(ordinary_birth_request_mutex);
			bool registered = false;
			for (auto *item = ordinary_birth_request_head; item; item = item->next_)
				registered |= item == this;
			if (!registered || phase_ != phase::granted || !producer_ || !source_ ||
			    !worker_ || worker_->generation() != generation_ ||
			    worker_->attempt() != attempt_)
				return false;
			worker = worker_;
		}
		// Never acquire coordinator_mutex while holding request mutex.
		return worker->current();
	}
	catch (...)
	{
		return false;
	}
}
const std::string *
quest_mobile_native_birth_ordinary_execution_lease::selected_root() const noexcept
{
	return current() ? &source_->state_->selected_root : nullptr;
}
void quest_mobile_native_birth_ordinary_execution_lease::cancel_pending() noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(ordinary_birth_request_mutex);
		for (auto *item = ordinary_birth_request_head; item; item = item->next_)
			if (item->phase_ == phase::requested || item->phase_ == phase::inspecting)
				item->phase_ = phase::refused;
		// A granted interval remains held until its actual worker RAII release.
		ordinary_birth_request_changed.notify_all();
	}
	catch (...)
	{
		std::terminate();
	}
}
size_t quest_mobile_native_birth_ordinary_execution_lease::fixed_storage_bytes() noexcept
{
	return sizeof(ordinary_birth_request_mutex) + sizeof(ordinary_birth_request_changed) +
	       sizeof(ordinary_birth_request_head);
}

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
	std::unique_ptr<quest_mobile_native_birth_ordinary_source_pin> ordinary_flat_source;
	bool ordinary_flat_replay_registered = false;
	std::string ordinary_flat_recovery_root;
	std::shared_ptr<const quest_mobile_native_npc_flat_factory_scope> npc_flat_factory_scope;
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
	original_reset_dispatch_cursor() = default;
	explicit original_reset_dispatch_cursor(const char *root, size_t length)
		: selected_flat_root(root, length)
	{
	}
	bool flat_backend = false;
	std::string selected_flat_root;
	critical_operation_id flat_lineage{}, flat_epoch{};
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
} // namespace

bool quest_mobile_native_birth_owner::reset_dispatch_identity() noexcept
{
	return reset_in_progress && replay_ready && nevent_is_game_thread() &&
	       (reset_dispatch.flat_backend ? reset_flat_projection_current() :
					      economic_gameplay_authority::active_regular_sql()) &&
	       zone_table && reset_zone_rnum >= 0 && reset_zone_rnum <= top_of_zone_table &&
	       reset_dispatch.commands &&
	       zone_table[reset_zone_rnum].cmd == reset_dispatch.commands &&
	       zone_table[reset_zone_rnum].number == reset_dispatch.zone_vnum;
}
bool quest_mobile_native_birth_owner::reset_dispatch_current() noexcept
{
	return reset_dispatch.valid && reset_dispatch_identity();
}
bool quest_mobile_native_birth_owner::original_command_current() noexcept
{
	if (!reset_dispatch_current() || !reset_dispatch.open)
		return false;
	const auto &a = reset_dispatch.commands[reset_dispatch.current_slot];
	const auto &b = reset_dispatch.original_command;
	return a.command == b.command && a.if_flag == b.if_flag && a.arg1 == b.arg1 &&
	       a.arg2 == b.arg2 && a.arg3 == b.arg3 && a.arg4 == b.arg4;
}
namespace
{
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
} // namespace

bool quest_mobile_native_birth_owner::reset_invocation_ready() noexcept
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
bool quest_mobile_native_birth_owner::reset_dispatch_source(uint32_t slot, char command, int room,
							    bool current,
							    economic_source_event *source,
							    int32_t *zone_vnum) noexcept
{
	if (!source || !zone_vnum || !reset_dispatch_current())
		return false;
	if (current ? (!reset_dispatch.open || reset_dispatch.completed ||
		       reset_dispatch.current_slot != slot ||
		       reset_dispatch.current_command != command) :
		      !(slot < reset_dispatch.next_slot ||
			(reset_dispatch.open && reset_dispatch.current_slot == slot)))
		return false;
	if (reset_dispatch.flat_backend && current && !original_command_current())
		return false; // Exact original flat command snapshot precedes nonce capture.
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

namespace
{
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

bool quest_mobile_native_birth_owner::reset_actor_holds_current() noexcept
{
	const auto actor_hold_current = [](const original_reset_dispatch_cursor::actor_hold &hold) noexcept
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
	};
	const auto &locals = reset_dispatch.locals;
	const P_char actors[] = { locals.mob, locals.last_mob, locals.tmp_mob,
				  locals.last_mob_followable };
	for (size_t i = 0; i < 4; ++i)
		if (reset_dispatch.actors[i].character != actors[i] ||
		    !actor_hold_current(reset_dispatch.actors[i]))
			return false;
	return true;
}
bool quest_mobile_native_birth_owner::reset_resume_current() noexcept
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
	return charge(size_t{ 0 });
}

bool quest_mobile_native_birth_owner::charge(size_t prospective_scratch) noexcept
{
	try
	{
		size_t bytes =
			quest_mobile_native_birth_ordinary_execution_lease::fixed_storage_bytes();
		if (reset_in_progress && !add_bytes(bytes, sizeof(reset_dispatch)))
			return false;
		if (reset_in_progress && reset_dispatch.selected_flat_root.capacity() > 15 &&
		    (reset_dispatch.selected_flat_root.capacity() == SIZE_MAX ||
		     !add_bytes(bytes, reset_dispatch.selected_flat_root.capacity() + 1)))
			return false;
		if (!add_bytes(bytes, births.capacity() * sizeof(births[0])) ||
		    !add_bytes(bytes, deferred.capacity() * sizeof(deferred[0])))
			return false;
		for (const auto &ptr : births)
			if (ptr)
			{
				const auto &b = *ptr;
				if (b.npc_flat_factory_scope)
				{
					const size_t retained =
						b.npc_flat_factory_scope->retained_heap_bytes();
					if (!retained || !add_bytes(bytes, retained))
						return false;
				}
				if (b.ordinary_flat_source &&
				    (!b.ordinary_flat_source->state_ ||
				     !add_bytes(bytes, sizeof(*b.ordinary_flat_source)) ||
				     !add_bytes(bytes, sizeof(*b.ordinary_flat_source->state_)) ||
				     (b.ordinary_flat_source->state_->selected_root.capacity() >
					      15 &&
				      (b.ordinary_flat_source->state_->selected_root.capacity() ==
					       SIZE_MAX ||
				       !add_bytes(bytes, b.ordinary_flat_source->state_
									 ->selected_root.capacity() +
								 1)))))
					return false;
				if (b.ordinary_flat_recovery_root.capacity() > 15 &&
				    (b.ordinary_flat_recovery_root.capacity() == SIZE_MAX ||
				     !add_bytes(bytes,
						b.ordinary_flat_recovery_root.capacity() + 1)))
					return false;
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
				if (item_native_quest_global_budget_scope_owner::literal_pool_owned())
				{
					// Paired genuine ROOT owns the whole mobile pool/catalog and
					// actual published NPC-only allocations. The stage owns only
					// its still-detached NPC-only body; mobile_consumed can lag
					// the real native list transfer during a nested request.
					size_t mobile_private = 0;
					if (!b.mobile.retained_bytes_excluding_mobile_pool(
						    &mobile_private) ||
					    !add_bytes(bytes, mobile_private))
						return false;
				}
				else if (!b.mobile_consumed && !add_bytes(bytes, b.mobile_bytes))
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
						size_t retained = 0;
						if (item_native_quest_global_budget_scope_owner::
							    literal_pool_owned() &&
						    item.stage->is_flat_factory())
						{
							if (!item.stage
								     ->retained_bytes_excluding_literal_pools(
									     &retained))
								return false;
						}
						else
							retained = item.stage->retained_bytes();
						if (!retained || !add_bytes(bytes, retained))
							return false;
					}
				}
			}
		size_t warm_bytes = 0;
		if (!zone_reset_item_owner::warm_retained_size(&warm_bytes) ||
		    !add_bytes(bytes, warm_bytes))
			return false;
		if (!add_bytes(bytes, prospective_scratch))
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
	const auto capture_actor_hold = [](P_char actor, original_reset_dispatch_cursor::actor_hold *output) noexcept
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
	};
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
		if (b.cash_role.role == native_mobile_birth_cash_role::ordinary_wallet &&
		    reset_dispatch.flat_backend && !capture_ordinary_flat_source_pin(index))
		{
			b.blocked = true;
			return;
		}
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
	service_ordinary_flat_execution_requests();
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
						if (b.npc_flat_factory_scope)
						{
							const size_t freeze_outer =
								sizeof(prepare_original_resets) +
								sizeof(recovery_only) + sizeof(i) +
								sizeof(&b) + sizeof(size_t);
							if (!freeze_ordinary_flat_command(
								    i, freeze_outer))
								continue;
						}
						else
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
									    b.image, b.recipes,
									    b.cash_role,
									    critical_source_site::
										    zone_event,
									    b.accepted_usec,
									    &original) !=
							    economic_accounting_error::ok)
								continue;
							std::vector<uint8_t> canonical;
							if (critical_command_encode(original,
										    &canonical) !=
							    critical_command_codec_result::ok)
								continue;
							b.command = std::move(original);
							b.canonical = std::move(canonical);
							if (!quest_mobile_native_birth_owner::
								    charge())
							{
								b.blocked = true;
								discard(i);
								continue;
							}
						}
					}

					if (b.envelope.attachment.empty())
					{
						if (b.npc_flat_factory_scope)
						{
							const size_t envelope_outer =
								sizeof(prepare_original_resets) +
								sizeof(recovery_only) + sizeof(i) +
								sizeof(&b) + sizeof(size_t);
							if (!prepare_ordinary_flat_envelope(
								    i, envelope_outer))
								continue;
						}
						else
						{
							native_mobile_birth_recovery_context context;
							context.items.reserve(b.stock.size());
							for (const auto &item : b.stock)
							{
								native_mobile_birth_recovery_item
									row;
								row.object_uid = item.uid;
								row.effects.resize(
									item.effects.size());
								context.items.push_back(
									std::move(row));
							}
							critical_native_recovery_envelope envelope;
							envelope.command = b.command;
							envelope.revision = 1;
							envelope.phase =
								critical_native_recovery_phase::
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
					}
					if (ordinary_wallet_command(b.command) &&
					    (b.npc_flat_factory_scope ?
						     !ordinary_flat_submission_role_current(
							     i, sizeof(prepare_original_resets) +
									sizeof(recovery_only) +
									sizeof(i) + sizeof(&b)) :
						     !ordinary_cash_role_current(b)))
						continue;
					bool submission_current = true;
					const auto result =
						b.npc_flat_factory_scope ?
							submit_ordinary_flat_envelope(
								i,
								sizeof(prepare_original_resets) +
									sizeof(recovery_only) +
									sizeof(i) + sizeof(&b) +
									sizeof(submission_current) +
									sizeof(critical_submit_result),
								&submission_current) :
							critical_native_mobile_birth_publication_owner::
								submit(b.envelope);
					if (critical_submit_result_keeps_operation(result))
						b.submitted = true;
					else if (result == critical_submit_result::invalid ||
						 result ==
							 critical_submit_result::identity_conflict)
						b.blocked = true;
					if (b.npc_flat_factory_scope && !submission_current)
						continue;
				}
				if (b.npc_flat_factory_scope)
				{
					if (b.submitted &&
					    (!b.coordinator_generation || !b.completed) &&
					    !observe_ordinary_flat_post_submit(
						    i, sizeof(prepare_original_resets) +
							       sizeof(recovery_only) + sizeof(i) +
							       sizeof(&b)))
						continue;
				}
				else
				{
					if (b.submitted && !b.coordinator_generation)
						critical_native_mobile_birth_publication_owner::
							observe_generation(
								b.envelope,
								&b.coordinator_generation);
					critical_completion completion{};
					if (b.submitted && !b.completed &&
					    critical_command_coordinator_get_completed(
						    b.command.operation_id, &completion))
					{
						if (!critical_completion_disposition_valid(
							    completion))
						{
							b.blocked = true;
							continue;
						}
						b.completion = completion;
						b.completed = true;
					}
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

// This observer reads today's actual installed projection only to reject drift
// against the retained capsule; it supplies no source or execution permission.
bool quest_mobile_native_birth_owner::reset_flat_projection_current() noexcept
{
	if (!reset_dispatch.flat_backend || reset_dispatch.selected_flat_root.empty())
		return false;
	const char *configured = persistence_mode_flatfile_root();
	if (!configured || reset_dispatch.selected_flat_root != configured)
		return false;
	size_t scratch = 2 * sizeof(critical_operation_id);
	const size_t observer = economic_gameplay_authority::active_regular_flat_working_bytes();
	if (!add_bytes(scratch, observer) || !charge(scratch))
		return false;
	bool matches = false;
	{
		critical_operation_id lineage{}, epoch{};
		matches = economic_gameplay_authority::capture_flat_reset_projection(&lineage,
										     &epoch) &&
			  lineage.bytes == reset_dispatch.flat_lineage.bytes &&
			  epoch.bytes == reset_dispatch.flat_epoch.bytes;
	}
	const bool recensused = charge(); // observer and local IDs have died.
	return matches && recensused;
}

bool quest_mobile_native_birth_owner::prepare_flat_reset_cursor(int zone, int force) noexcept
{
	if (!nevent_is_game_thread() || !replay_ready || reset_in_progress || !zone_table ||
	    zone < 0 || zone > top_of_zone_table || !zone_table[zone].cmd)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)force;
	return false;
#else
	const char *configured = persistence_mode_flatfile_root();
	if (!configured || !*configured)
		return false;
	const size_t length = std::char_traits<char>::length(configured);
	size_t root_heap = 0;
	if (length > 15)
	{
		if (length == SIZE_MAX)
			return false;
		root_heap = length + 1; // actual fresh libstdc++13 string construction.
	}
	const size_t observer = economic_gameplay_authority::active_regular_flat_working_bytes();
	size_t peak = 2 * sizeof(critical_operation_id);
	if (!add_bytes(peak, observer) || !charge(peak))
		return false;
	try
	{
		critical_operation_id lineage{}, epoch{};
		if (!economic_gameplay_authority::capture_flat_reset_projection(&lineage, &epoch))
			return false;
		peak = 2 * sizeof(critical_operation_id);
		if (!add_bytes(peak, sizeof(original_reset_dispatch_cursor)) ||
		    !add_bytes(peak, root_heap) || !charge(peak))
			return false;
		original_reset_dispatch_cursor candidate(configured, length);
		candidate.flat_backend = true;
		candidate.flat_lineage = lineage;
		candidate.flat_epoch = epoch;
		candidate.commands = zone_table[zone].cmd;
		candidate.zone_vnum = zone_table[zone].number;
		candidate.force = force;
		candidate.valid = true;
		// Prove projection/root after the fallible copy, before publishing a cursor.
		if (!add_bytes(peak, 2 * sizeof(critical_operation_id)) ||
		    !add_bytes(peak, observer) || !charge(peak))
			return false;
		critical_operation_id actual_lineage{}, actual_epoch{};
		const char *actual_root = persistence_mode_flatfile_root();
		if (!economic_gameplay_authority::capture_flat_reset_projection(&actual_lineage,
										&actual_epoch) ||
		    actual_lineage.bytes != lineage.bytes || actual_epoch.bytes != epoch.bytes ||
		    !actual_root || candidate.selected_flat_root != actual_root ||
		    zone_table[zone].cmd != candidate.commands ||
		    zone_table[zone].number != candidate.zone_vnum)
			return false;
		static_assert(std::is_nothrow_move_assignable_v<original_reset_dispatch_cursor>);
		reset_dispatch = std::move(candidate);
		return true; // All temporary DTOs die before caller's retained recensus.
	}
	catch (...)
	{
		return false;
	}
#endif
}

// Deliberately private and not called by the existing begin_reset front door.
// The real db dispatcher must first supply a guarded flat O/P/S cursor route;
// exposing this early would use inactive locals and ordinary M/G/E branches.
bool quest_mobile_native_birth_owner::begin_reset_flat(int zone, int force) noexcept
{
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return false;
	// Match the original active-projection gate before even queueing values.
	size_t observer_live = 2 * sizeof(critical_operation_id);
	if (!add_bytes(observer_live,
		       economic_gameplay_authority::active_regular_flat_working_bytes()) ||
	    !charge(observer_live))
		return false;
	bool selected = false;
	{
		critical_operation_id lineage{}, epoch{};
		selected = economic_gameplay_authority::capture_flat_reset_projection(&lineage,
										      &epoch);
	}
	if (!charge() || !selected)
		return false;
	// Reentry keeps the actual selected backend/root/projection and original
	// known-pure hold. It cannot reinterpret an existing SQL cursor as flat.
	if (reset_dispatch.resume_dispatch)
		return reset_dispatch.flat_backend && zone == reset_zone_rnum &&
		       force == reset_dispatch.force && reset_resume_current() &&
		       reset_objects_current() && charge();
	bool held = !replay_ready || reset_in_progress;
	if (zone_table && zone >= 0 && zone <= top_of_zone_table &&
	    zone_reset_room_item_warm_pending_vnum(zone_table[zone].number))
		held = true;
	for (const auto &b : births)
		if (b && !b->retired && b->zone == zone)
			held = true;
	if (held)
	{
		const auto found = std::find_if(deferred.begin(), deferred.end(), [&](const auto &r)
						{ return r.zone == zone && r.force == force; });
		if (found == deferred.end())
		{
			if (deferred.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS)
			{
				deferred_overflow = true;
				return false;
			}
			size_t extra = 0;
			if (deferred.size() == deferred.capacity())
			{
				size_t count = deferred.size();
				if (std::max(count, size_t{ 1 }) > SIZE_MAX - count)
					return false;
				count += std::max(count, size_t{ 1 });
				if (count > SIZE_MAX / sizeof(original_reset_request))
					return false;
				extra = count * sizeof(original_reset_request);
			}
			if (!add_bytes(extra, sizeof(original_reset_request)) || !charge(extra))
				return false;
			try
			{
				deferred.push_back({ zone, force });
			}
			catch (...)
			{
				(void)charge();
				deferred_overflow = true;
				return false;
			}
			if (!charge())
				deferred_overflow = true;
		}
		return false;
	}
	const bool prepared = prepare_flat_reset_cursor(zone, force);
	if (!prepared)
	{
		(void)charge();
		return false;
	}
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

bool quest_mobile_native_birth_owner::capture_reset_backend(
	const economic_source_event &source, const std::string **selected_flat_root) noexcept
{
	if (!selected_flat_root || !reset_dispatch_current() ||
	    critical_operation_id_is_zero(reset_invocation) ||
	    source.kind != economic_source_kind::world_generation ||
	    source.source.bytes != reset_invocation.bytes ||
	    source.generation.bytes != reset_invocation.bytes || source.sequence ||
	    !(source.slot < reset_dispatch.next_slot ||
	      (reset_dispatch.open && source.slot == reset_dispatch.current_slot) ||
	      (reset_dispatch.completed && source.slot == reset_dispatch.current_slot)))
		return false;
	const char command = reset_dispatch.commands[source.slot].command;
	if (command != 'O' && command != 'P' && command != 'S')
		return false;
	if (reset_dispatch.open && source.slot == reset_dispatch.current_slot &&
	    !original_command_current())
		return false;
	if (command == 'S' &&
	    (!reset_dispatch.completed || source.slot != reset_dispatch.current_slot))
		return false;
	*selected_flat_root = reset_dispatch.flat_backend ? &reset_dispatch.selected_flat_root :
							    nullptr;
	return true;
}

bool quest_mobile_native_birth_owner::capture_reset_flat_projection(
	const economic_source_event &source, critical_operation_id *lineage,
	critical_operation_id *epoch) noexcept
{
	if (!lineage || !epoch || lineage == epoch)
		return false;
	const std::string *root = nullptr;
	if (!capture_reset_backend(source, &root) || !root)
		return false;
	*lineage = reset_dispatch.flat_lineage;
	*epoch = reset_dispatch.flat_epoch;
	return true;
}

namespace
{
bool birth_passive_add(size_t &value, size_t extra) noexcept
{
	if (value > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    extra > PLAYER_SAVE_PIPELINE_MAX_BYTES - value)
		return false;
	value += extra;
	return true;
}
bool birth_passive_rows(size_t &value, size_t count, size_t width) noexcept
{
	return (!width || count <= PLAYER_SAVE_PIPELINE_MAX_BYTES / width) &&
	       birth_passive_add(value, count * width);
}
bool birth_passive_admit(size_t current, size_t request, bool (*reserve)(size_t, void *) noexcept,
			 void *context) noexcept
{
	// Actual value carriers remain alive during the genuine callback. These
	// explicit object bytes are separate from the caller's live workspace.
	return reserve && birth_passive_add(current, request) &&
	       birth_passive_add(current, sizeof(current) + sizeof(request) + sizeof(reserve) +
						  sizeof(context)) &&
	       reserve(current, context);
}
bool birth_passive_command_heap(size_t &value, const critical_command &command,
				bool fresh_copy = false) noexcept
{
	return birth_passive_rows(value, fresh_copy ? command.keys.size() : command.keys.capacity(),
				  sizeof(critical_entity_key)) &&
	       birth_passive_rows(value,
				  fresh_copy ? command.expected_revisions.size() :
					       command.expected_revisions.capacity(),
				  sizeof(critical_expected_revision)) &&
	       birth_passive_add(value, fresh_copy ? command.payload.size() :
						     command.payload.capacity()) &&
	       birth_passive_add(value, fresh_copy ? command.accounting_intent.size() :
						     command.accounting_intent.capacity());
}
bool birth_passive_envelope_heap(size_t &value, const critical_native_recovery_envelope &envelope,
				 bool fresh_copy = false) noexcept
{
	return birth_passive_command_heap(value, envelope.command, fresh_copy) &&
	       birth_passive_add(value, fresh_copy ? envelope.attachment.size() :
						     envelope.attachment.capacity());
}
bool birth_passive_context_heap(size_t &value,
				const native_mobile_birth_recovery_context &progress) noexcept
{
	if (!birth_passive_rows(value, progress.items.capacity(),
				sizeof(native_mobile_birth_recovery_item)))
		return false;
	for (const auto &item : progress.items)
		if (!birth_passive_rows(value, item.effects.capacity(),
					sizeof(native_mobile_birth_recovery_effect)))
			return false;
	return true;
}
bool birth_passive_text_heap(size_t &value, const std::string &text) noexcept
{
	return text.capacity() <= 15 ||
	       (text.capacity() != SIZE_MAX && birth_passive_add(value, text.capacity() + 1));
}
bool birth_passive_image_heap(size_t &value, const quest_mobile_native_image &image) noexcept
{
	if (!birth_passive_rows(value, image.items.capacity(), sizeof(player_item_snapshot)))
		return false;
	for (const auto &item : image.items)
	{
		if (!birth_passive_text_heap(value, item.name) ||
		    !birth_passive_text_heap(value, item.short_description) ||
		    !birth_passive_text_heap(value, item.description) ||
		    !birth_passive_text_heap(value, item.action_description) ||
		    !birth_passive_rows(value, item.dynamic_affects.capacity(),
					sizeof(player_item_dynamic_affect_snapshot)) ||
		    !birth_passive_rows(value, item.extra_descriptions.capacity(),
					sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &description : item.extra_descriptions)
			if (!birth_passive_text_heap(value, description.keyword) ||
			    !birth_passive_text_heap(value, description.description) ||
			    !birth_passive_rows(value, description.spell_ids.capacity(),
						sizeof(int32_t)))
				return false;
	}
	return true;
}
bool birth_passive_recipes_heap(
	size_t &value, const std::vector<native_mobile_birth_item_recipe> &recipes) noexcept
{
	if (!birth_passive_rows(value, recipes.capacity(), sizeof(native_mobile_birth_item_recipe)))
		return false;
	for (const auto &recipe : recipes)
		if (!birth_passive_rows(value, recipe.libraries.capacity(),
					sizeof(native_mobile_birth_library_recipe)))
			return false;
	return true;
}
bool birth_passive_stock_heap(size_t &value, const std::vector<original_item> &stock) noexcept
{
	if (!birth_passive_rows(value, stock.capacity(), sizeof(original_item)))
		return false;
	for (const auto &item : stock)
		if (!birth_passive_rows(value, item.effects.capacity(),
					sizeof(quest_mobile_native_item_effect)))
			return false;
	return true;
}
// Closed owning state: only the new passive original_birth constructed below,
// before registry transfer. No factory, binding, affect/checkpoint writer or
// native stage is initialized; those original default members own no heap.
bool birth_passive_new_body_heap(size_t &value, const original_birth &body) noexcept
{
	return birth_passive_add(value, sizeof(body)) &&
	       birth_passive_text_heap(value, body.ordinary_flat_recovery_root) &&
	       birth_passive_image_heap(value, body.image) &&
	       birth_passive_recipes_heap(value, body.recipes) &&
	       birth_passive_command_heap(value, body.command) &&
	       birth_passive_add(value, body.canonical.capacity()) &&
	       (!body.shared_checkpoint ||
		(birth_passive_add(value, sizeof(*body.shared_checkpoint)) &&
		 birth_passive_add(value, body.shared_checkpoint->capacity()))) &&
	       birth_passive_rows(value, body.current_custody.capacity(),
				  sizeof(item_ownership_runtime_entry)) &&
	       birth_passive_stock_heap(value, body.stock) &&
	       birth_passive_envelope_heap(value, body.envelope) &&
	       birth_passive_context_heap(value, body.recovery);
}
template <typename T>
bool birth_passive_push_request(const std::vector<T> &values, size_t *request) noexcept
{
	if (!request || values.size() > values.capacity())
		return false;
	if (values.size() < values.capacity())
	{
		*request = 0;
		return true;
	}
	// Actual supported GCC13 _M_check_len(1): size+max(size,1), limited by
	// this real default-allocator vector's own max_size. Current old heap remains.
	if (values.size() == values.max_size())
		return false;
	const size_t increment = std::max(values.size(), size_t{ 1 });
	const size_t capacity = increment > values.max_size() - values.size() ?
					values.max_size() :
					values.size() + increment;
	size_t bytes = 0;
	if (!birth_passive_rows(bytes, capacity, sizeof(T)))
		return false;
	*request = bytes;
	return true;
}
bool birth_passive_same_image(const quest_mobile_native_image &left,
			      const quest_mobile_native_image &right,
			      bool (*reserve)(size_t, void *) noexcept, void *context,
			      size_t outer) noexcept
{
	struct image_comparison_workspace
	{
		std::vector<uint8_t> left, right;
		size_t current;
	};
	size_t base = outer;
	if (!birth_passive_add(base, sizeof(image_comparison_workspace)) ||
	    !birth_passive_add(base, sizeof(base)) ||
	    !birth_passive_add(base, sizeof(reserve) + sizeof(context) + sizeof(outer)) ||
	    !birth_passive_admit(base, 0, reserve, context))
		return false;
	image_comparison_workspace work;
	if (quest_mobile_native_image_encode_bounded(left, &work.left, reserve, context, base) !=
	    player_snapshot_codec_result::ok)
		return false;
	work.current = base;
	return birth_passive_add(work.current, work.left.capacity()) &&
	       quest_mobile_native_image_encode_bounded(right, &work.right, reserve, context,
							work.current) ==
		       player_snapshot_codec_result::ok &&
	       work.left == work.right;
}
struct birth_passive_command_workspace
{
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	quest_mobile_native_constructor_recipe constructor;
	std::vector<uint8_t> canonical;
	std::unique_ptr<original_birth> body;
	original_item row;
	size_t base, current, request, available, scan, item_scan, old_scan;
};
bool birth_passive_command_current(const birth_passive_command_workspace &work,
				   size_t *output) noexcept
{
	size_t current = work.base;
	if (!output || !birth_passive_image_heap(current, work.image) ||
	    !birth_passive_recipes_heap(current, work.recipes) ||
	    !birth_passive_add(current, work.canonical.capacity()) ||
	    !birth_passive_rows(current, work.row.effects.capacity(),
				sizeof(quest_mobile_native_item_effect)) ||
	    (work.body && !birth_passive_new_body_heap(current, *work.body)))
		return false;
	*output = current;
	return true;
}
} // genuine passive allocations only

struct quest_mobile_native_birth_owner::shared_shop_restore_attachment
{
	original_birth *identity = nullptr;
	size_t slot = SIZE_MAX;
	bool newly_attached = false;
};

bool quest_mobile_native_birth_owner::restore_shared_shop_command_bounded(
	const critical_command &command, const std::vector<uint8_t> &checkpoint,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	shared_shop_restore_attachment *attached) noexcept
{
	if (command.type != critical_command_type::native_mobile_birth ||
	    !shared_shop_command(command) || checkpoint.empty() || !attached)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t base = outer;
	if (!birth_passive_add(base, sizeof(birth_passive_command_workspace)) ||
	    !birth_passive_add(base, sizeof(base)) ||
	    !birth_passive_add(base, sizeof(reserve) + sizeof(context) + sizeof(outer) +
					     sizeof(attached)) ||
	    !birth_passive_admit(base, 0, reserve, context))
		return false;
	birth_passive_command_workspace work;
	work.base = base;
	try
	{
		if (native_mobile_birth_cash_role_command_decode_bounded(
			    command, &work.image, &work.recipes, &work.role, reserve, context,
			    work.base) != economic_accounting_error::ok ||
		    work.role.role != native_mobile_birth_cash_role::shared_shopkeeper)
			return false;
		work.constructor = work.role.original;
		if (!birth_passive_command_current(work, &work.current) ||
		    critical_command_encode_bounded(command, &work.canonical, reserve, context,
						    work.current) !=
			    critical_command_codec_result::ok)
			return false;
		for (work.scan = 0; work.scan < births.size(); ++work.scan)
			if (births[work.scan] &&
			    births[work.scan]->reference.birth_operation.bytes ==
				    command.operation_id.bytes)
			{
				const auto &previous = *births[work.scan];
				return previous.canonical == work.canonical &&
				       birth_passive_command_current(work, &work.current) &&
				       birth_passive_same_image(previous.image, work.image, reserve,
								context, work.current) &&
				       previous.shared_checkpoint &&
				       *previous.shared_checkpoint == checkpoint;
			}
		for (work.scan = 0; work.scan < births.size(); ++work.scan)
			if (births[work.scan])
			{
				const auto &previous = *births[work.scan];
				const auto &left = previous.reference.birth_source;
				const auto &right = work.image.reference.birth_source;
				if (previous.reference.mobile_instance_id ==
					    work.image.reference.mobile_instance_id ||
				    (left.kind == right.kind &&
				     left.source.bytes == right.source.bytes &&
				     left.generation.bytes == right.generation.bytes &&
				     left.sequence == right.sequence && left.slot == right.slot))
					return false;
				for (work.item_scan = 0; work.item_scan < work.image.items.size();
				     ++work.item_scan)
					for (work.old_scan = 0;
					     work.old_scan < previous.image.items.size();
					     ++work.old_scan)
						if (work.image.items[work.item_scan].object_uid ==
						    previous.image.items[work.old_scan].object_uid)
							return false;
			}
		work.available = 0;
		while (work.available < births.size() && births[work.available])
			++work.available;
		if (work.available == births.size() &&
		    births.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS)
			return false;
		if (!birth_passive_command_current(work, &work.current) ||
		    !birth_passive_admit(work.current,
					 sizeof(original_birth) +
						 sizeof(std::unique_ptr<original_birth>),
					 reserve, context))
			return false;
		work.body = std::make_unique<original_birth>();
		static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_image>);
		static_assert(std::is_nothrow_move_assignable_v<
			      std::vector<native_mobile_birth_item_recipe>>);
		static_assert(std::is_nothrow_move_constructible_v<original_item>);
		work.body->reference = work.image.reference;
		work.body->image = std::move(work.image);
		work.body->recipes = std::move(work.recipes);
		work.body->constructor = work.constructor;
		work.body->constructor_present = true;
		work.body->cash_role = work.role;
		work.body->cash_role_present = true;
		work.request = sizeof(std::vector<uint8_t>) +
			       sizeof(std::unique_ptr<const std::vector<uint8_t>>);
		if (!birth_passive_add(work.request, checkpoint.size()) ||
		    !birth_passive_command_current(work, &work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		work.body->shared_checkpoint =
			std::make_unique<const std::vector<uint8_t>>(checkpoint);
		if (work.constructor.wire_version ==
			    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
		    work.constructor.wire_version ==
			    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
		{
			if (work.constructor.reset_room_vnum !=
			    work.body->reference.birthplace_vnum)
				return false;
			work.body->shop = work.constructor.reset_shop_index;
		}
		work.request = 0;
		if (!birth_passive_command_heap(work.request, command, true) ||
		    !birth_passive_command_current(work, &work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		work.body->command = command;
		work.body->canonical = std::move(work.canonical);
		work.body->rnum = real_mobile(work.body->reference.mobile_vnum);
		work.body->zone = real_zone(work.body->reference.reset_zone_vnum);
		work.body->room = real_room0(work.body->reference.birthplace_vnum);
		if (work.body->rnum < 0 || work.body->zone < 0 || work.body->room < 0)
			return false;
		work.body->cold = true;
		work.body->cold_replay_enrolled = true;
		work.body->sealed = true;
		work.body->submitted = true;
		work.request = 0;
		if (!birth_passive_rows(work.request, work.body->image.items.size(),
					sizeof(item_ownership_runtime_entry)) ||
		    !birth_passive_command_current(work, &work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		work.body->current_custody.resize(work.body->image.items.size());
		for (work.item_scan = 0; work.item_scan < work.body->image.items.size();
		     ++work.item_scan)
		{
			work.row.uid = work.body->image.items[work.item_scan].object_uid;
			work.row.rnum = real_object(work.body->image.items[work.item_scan].vnum);
			if (work.row.rnum < 0)
				return false;
			if (!birth_passive_push_request(work.body->stock, &work.request) ||
			    !birth_passive_command_current(work, &work.current) ||
			    !birth_passive_admit(work.current, work.request, reserve, context))
				return false;
			work.body->stock.push_back(std::move(work.row));
		}
		if (work.available == births.size())
		{
			if (!birth_passive_push_request(births, &work.request) ||
			    !birth_passive_command_current(work, &work.current) ||
			    !birth_passive_admit(work.current, work.request, reserve, context))
				return false;
			births.push_back(std::move(work.body));
		}
		else
			births[work.available] = std::move(work.body);
		// Body heaps transferred once to genuine retained birth ownership. Current
		// provider now excludes that body; registry capacity is observed by charge.
		if (!birth_passive_command_current(work, &work.current) ||
		    !birth_passive_admit(work.current, 0, reserve, context))
		{
			births[work.available].reset();
			if (birth_passive_command_current(work, &work.current))
				(void)birth_passive_admit(work.current, 0, reserve, context);
			return false;
		}
		attached->identity = births[work.available].get();
		attached->slot = work.available;
		attached->newly_attached = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_mobile_native_birth_owner::restore_shared_shop_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer) noexcept
{
	if (envelope.command.type != critical_command_type::native_mobile_birth ||
	    !shared_shop_command(envelope.command))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	struct restore_workspace
	{
		native_mobile_birth_shared_shop_recovery_context shared;
		std::span<const uint8_t> attachment;
		critical_native_recovery_envelope retained;
		quest_mobile_native_image image;
		std::vector<uint8_t> canonical;
		std::vector<original_item> stock;
		original_item row;
		shared_shop_restore_attachment attached;
		size_t base, current, request, scan, row_scan, image_scan;
		bool current_bytes(size_t *output) const noexcept
		{
			size_t live = base;
			if (!output || !birth_passive_context_heap(live, shared.progress) ||
			    !birth_passive_add(live, shared.original_checkpoint.capacity()) ||
			    !birth_passive_envelope_heap(live, retained) ||
			    !birth_passive_image_heap(live, image) ||
			    !birth_passive_add(live, canonical.capacity()) ||
			    !birth_passive_stock_heap(live, stock) ||
			    !birth_passive_rows(live, row.effects.capacity(),
						sizeof(quest_mobile_native_item_effect)))
				return false;
			*output = live;
			return true;
		}
	};
	size_t base = outer;
	if (!birth_passive_add(base, sizeof(restore_workspace)) ||
	    !birth_passive_add(base, sizeof(base)) ||
	    !birth_passive_add(base, sizeof(reserve) + sizeof(context) + sizeof(outer)) ||
	    !birth_passive_admit(base, 0, reserve, context))
		return false;
	restore_workspace work;
	work.base = base;
	work.attachment = envelope.attachment;
	try
	{
		if (!native_mobile_birth_shared_shop_recovery_valid_bounded(envelope, reserve,
									    context, work.base) ||
		    native_mobile_birth_shared_shop_recovery_decode_bounded(
			    envelope.command, work.attachment, &work.shared, reserve, context,
			    work.base) != economic_accounting_error::ok)
			return false;
		work.request = 0;
		if (!birth_passive_envelope_heap(work.request, envelope, true) ||
		    !work.current_bytes(&work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		work.retained = envelope; // Exact original authenticated copy.
		for (work.scan = 0; work.scan < births.size(); ++work.scan)
			if (births[work.scan] &&
			    births[work.scan]->reference.birth_operation.bytes ==
				    envelope.command.operation_id.bytes)
			{
				const auto &entry = *births[work.scan];
				if (!entry.shared_checkpoint ||
				    *entry.shared_checkpoint != work.shared.original_checkpoint ||
				    entry.envelope.revision != envelope.revision ||
				    entry.envelope.phase != envelope.phase ||
				    entry.envelope.attachment != envelope.attachment)
					return false;
				return work.current_bytes(&work.current) &&
				       critical_command_encode_bounded(envelope.command,
								       &work.canonical, reserve,
								       context, work.current) ==
					       critical_command_codec_result::ok &&
				       entry.canonical == work.canonical;
			}
		{
			// The original nested recipes/role scope ends before stock construction.
			// This real output frame is admitted before construction and owns its
			// decoded library heaps until the complete codec returns.
			struct outer_decode_workspace
			{
				std::vector<native_mobile_birth_item_recipe> recipes;
				native_mobile_birth_cash_role_recipe role;
			};
			if (!work.current_bytes(&work.current) ||
			    !birth_passive_add(work.current, sizeof(outer_decode_workspace)) ||
			    !birth_passive_admit(work.current, 0, reserve, context))
				return false;
			outer_decode_workspace decoded;
			if (native_mobile_birth_cash_role_command_decode_bounded(
				    envelope.command, &work.image, &decoded.recipes, &decoded.role,
				    reserve, context,
				    work.current) != economic_accounting_error::ok ||
			    decoded.role.role != native_mobile_birth_cash_role::shared_shopkeeper)
				return false;
		}
		work.request = 0;
		if (!birth_passive_rows(work.request, work.shared.progress.items.size(),
					sizeof(original_item)) ||
		    !work.current_bytes(&work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		work.stock.reserve(work.shared.progress.items.size());
		for (work.row_scan = 0; work.row_scan < work.shared.progress.items.size();
		     ++work.row_scan)
		{
			work.image_scan = 0;
			while (work.image_scan < work.image.items.size() &&
			       work.image.items[work.image_scan].object_uid !=
				       work.shared.progress.items[work.row_scan].object_uid)
				++work.image_scan;
			if (work.image_scan == work.image.items.size())
				return false;
			work.row.uid = work.image.items[work.image_scan].object_uid;
			work.row.rnum = real_object(work.image.items[work.image_scan].vnum);
			if (work.row.rnum < 0)
				return false;
			if (!birth_passive_push_request(work.stock, &work.request) ||
			    !work.current_bytes(&work.current) ||
			    !birth_passive_admit(work.current, work.request, reserve, context))
				return false;
			work.stock.push_back(std::move(work.row));
		}
		if (!work.current_bytes(&work.current) ||
		    !restore_shared_shop_command_bounded(envelope.command,
							 work.shared.original_checkpoint, reserve,
							 context, work.current, &work.attached))
			return false;
		for (work.scan = 0; work.scan < births.size(); ++work.scan)
			if (births[work.scan] &&
			    births[work.scan]->reference.birth_operation.bytes ==
				    envelope.command.operation_id.bytes)
			{
				auto &entry = births[work.scan];
				static_assert(std::is_nothrow_move_assignable_v<
					      std::vector<original_item>>);
				static_assert(std::is_nothrow_move_assignable_v<
					      critical_native_recovery_envelope>);
				static_assert(std::is_nothrow_move_assignable_v<
					      native_mobile_birth_recovery_context>);
				entry->stock = std::move(work.stock);
				entry->envelope = std::move(work.retained);
				entry->recovery = std::move(work.shared.progress);
				if (!work.current_bytes(&work.current) ||
				    !birth_passive_admit(work.current, 0, reserve, context))
				{
					// An original duplicate returns before this attachment path.
					// Remove only the authentic body newly attached by this owner.
					if (work.attached.newly_attached &&
					    work.attached.slot == work.scan &&
					    work.attached.identity == entry.get())
						entry.reset();
					if (work.current_bytes(&work.current))
						(void)birth_passive_admit(work.current, 0, reserve,
									  context);
					return false;
				}
				return true; // Passive values only; native cold services remain gated.
			}
		if (work.attached.newly_attached && work.attached.slot < births.size() &&
		    births[work.attached.slot].get() == work.attached.identity)
		{
			births[work.attached.slot].reset();
			if (work.current_bytes(&work.current))
				(void)birth_passive_admit(work.current, 0, reserve, context);
		}
		return false;
	}
	catch (...)
	{
		if (work.attached.newly_attached && work.attached.slot < births.size() &&
		    births[work.attached.slot].get() == work.attached.identity)
		{
			births[work.attached.slot].reset();
			if (work.current_bytes(&work.current))
				(void)birth_passive_admit(work.current, 0, reserve, context);
		}
		return false;
	}
#endif
}

bool quest_mobile_native_birth_restore_shared_shop_bounded(
	const critical_native_recovery_envelope &envelope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	size_t full = outer_live;
	return birth_passive_add(full, sizeof(full) + sizeof(reserve) + sizeof(context) +
					       sizeof(outer_live)) &&
	       birth_passive_admit(full, 0, reserve, context) &&
	       quest_mobile_native_birth_owner::restore_shared_shop_bounded(envelope, reserve,
									    context, full);
}

bool quest_mobile_native_birth_owner::capture_ordinary_flat_source_pin(size_t index) noexcept
{
	if (!nevent_is_game_thread() || index >= births.size() || !births[index] ||
	    !reset_dispatch_current() || !reset_dispatch.flat_backend ||
	    reset_dispatch.selected_flat_root.empty())
		return false;
	auto &b = *births[index];
	const auto &source = b.reference.birth_source;
	if (b.ordinary_flat_source || b.cold || b.submitted || b.blocked || !b.sealed ||
	    !b.character || b.mobile.character() != b.character ||
	    b.character->runtime_id != b.runtime_id || !ordinary_cash_role_current(b) ||
	    b.zone != reset_zone_rnum || b.room < 0 || b.rnum < 0 ||
	    source.kind != economic_source_kind::npc_generation || source.sequence ||
	    source.source.bytes != reset_invocation.bytes ||
	    source.generation.bytes != reset_invocation.bytes ||
	    critical_operation_id_is_zero(reset_invocation) ||
	    critical_operation_id_is_zero(b.reference.birth_operation) ||
	    b.reference.birth_operation.bytes == reset_invocation.bytes ||
	    !b.reference.mobile_instance_id || b.reference.mobile_instance_id == UINT64_MAX ||
	    !(source.slot < reset_dispatch.next_slot ||
	      (reset_dispatch.open && source.slot == reset_dispatch.current_slot)) ||
	    b.reference.reset_zone_vnum != reset_dispatch.zone_vnum)
		return false;
	const auto &m = reset_dispatch.commands[source.slot];
	if (m.command != 'M' || m.arg1 != b.rnum || m.arg3 != b.room)
		return false;
	try
	{
		using pin = quest_mobile_native_birth_ordinary_source_pin;
		auto state = std::make_unique<pin::implementation>();
		state->producer = &b;
		state->selected_root = reset_dispatch.selected_flat_root;
		state->invocation = reset_invocation;
		state->birth = b.reference.birth_operation;
		state->lineage = reset_dispatch.flat_lineage;
		state->epoch = reset_dispatch.flat_epoch;
		state->instance = b.reference.mobile_instance_id;
		state->runtime = b.runtime_id;
		state->slot = source.slot;
		state->zone = b.zone;
		state->room = b.room;
		state->rnum = b.rnum;
		state->original_m_args = { m.arg1, m.arg2, m.arg3, m.arg4 };
		std::unique_ptr<pin> retained(new pin(std::move(state)));
		// Installation belongs only to this original factory; replay cannot mint it.
		b.ordinary_flat_source = std::move(retained);
		if (!charge())
		{
			b.ordinary_flat_source.reset();
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

bool quest_mobile_native_birth_owner::ordinary_flat_execution_source_current(
	const quest_mobile_native_birth_ordinary_execution_lease &request,
	const void *candidate) noexcept
{
	// Main-game-thread only; no request/coordinator/storage locks held.
	if (!nevent_is_game_thread() || !request.worker_ || !candidate ||
	    !request.worker_->borrowed_current() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	    persistence_mode_requires_mysql())
		return false;
	const auto found = std::find_if(births.begin(), births.end(), [&](const auto &entry)
					{ return entry.get() == candidate; });
	if (found == births.end() || !*found)
		return false;
	auto &b = **found;
	if (!b.ordinary_flat_source || !b.ordinary_flat_source->state_ || !b.submitted ||
	    !b.sealed || b.cold || b.blocked || b.completed || b.retired || b.mobile_started ||
	    b.mobile_consumed || b.runtime_applied || b.physically_proven || !b.character ||
	    b.mobile.character() != b.character || b.character->runtime_id != b.runtime_id ||
	    find_character_by_runtime_id(b.runtime_id) || !ordinary_cash_role_current(b) ||
	    !birth_recovery_valid(b.envelope) ||
	    b.envelope.phase != critical_native_recovery_phase::execution_pending ||
	    b.envelope.revision != request.worker_->revision() ||
	    b.envelope.phase != request.worker_->phase() ||
	    b.envelope.attachment.size() != request.worker_->attachment().size() ||
	    !std::equal(b.envelope.attachment.begin(), b.envelope.attachment.end(),
			request.worker_->attachment().begin()) ||
	    !critical_command_equal(b.command, request.worker_->command()) ||
	    !critical_command_equal(b.envelope.command, b.command))
		return false;
	const auto &pin = *b.ordinary_flat_source->state_;
	const auto &source = b.reference.birth_source;
	const char *configured = persistence_mode_flatfile_root();
	if (pin.producer != &b || !configured || pin.selected_root != configured ||
	    pin.runtime != b.runtime_id || pin.instance != b.reference.mobile_instance_id ||
	    pin.birth.bytes != b.reference.birth_operation.bytes || pin.slot != source.slot ||
	    pin.zone != b.zone || pin.room != b.room || pin.rnum != b.rnum ||
	    pin.original_m_args[0] != b.rnum || pin.original_m_args[2] != b.room ||
	    source.kind != economic_source_kind::npc_generation || source.sequence ||
	    source.source.bytes != pin.invocation.bytes ||
	    source.generation.bytes != pin.invocation.bytes)
		return false;
	try
	{
		uint64_t generation = 0;
		if (!critical_native_mobile_birth_publication_owner::observe_generation(
			    b.envelope, &generation) ||
		    generation != request.generation_ ||
		    (b.coordinator_generation && b.coordinator_generation != generation))
			return false;
		economic_frozen_intent intent;
		quest_mobile_native_image decoded;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		if (economic_intent_decode(b.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(b.command, intent) !=
			    economic_accounting_error::ok ||
		    intent.admission.metadata.lineage.bytes != pin.lineage.bytes ||
		    intent.admission.metadata.epoch.bytes != pin.epoch.bytes ||
		    !intent.admission.metadata.source_event.has_value() ||
		    intent.admission.metadata.source_event->kind != source.kind ||
		    intent.admission.metadata.source_event->source.bytes != source.source.bytes ||
		    intent.admission.metadata.source_event->generation.bytes !=
			    source.generation.bytes ||
		    intent.admission.metadata.source_event->sequence != source.sequence ||
		    intent.admission.metadata.source_event->slot != source.slot ||
		    native_mobile_birth_cash_role_command_decode(b.command, &decoded, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    !same_image(decoded, b.image) || !b.bindings.valid())
			return false;
		quest_mobile_native_constructor_digest build{}, procedure{}, tail{};
		if (!native_mobile_birth_running_artifact_digest(&build) ||
		    build != b.constructor.build_digest ||
		    !native_mobile_birth_procedure_capture(b.reference.mobile_vnum, build,
							   &procedure) ||
		    procedure != b.constructor.procedure_after ||
		    !native_mobile_birth_reset_tail_capture(
			    b.reference.mobile_vnum, b.reference.birthplace_vnum, b.shop, &tail) ||
		    tail != b.constructor.reset_tail)
			return false;
		quest_mobile_native_image actual;
		if (quest_mobile_native_capture(b.character, b.reference,
						quest_mobile_lifetime_state::live,
						b.reference.birth_operation, 1,
						&actual) != player_snapshot_capture_result::ok ||
		    !same_image(actual, b.image))
			return false;
		// Original publication/cold-adoption corruption and identity exclusions.
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
		for (P_char mob = character_list; mob; mob = mob->next)
		{
			if (mob == b.character || mob->runtime_id == b.runtime_id)
				return false;
			const auto *bytes = mob->native_mobile_binding.encoded_reference_;
			if (std::all_of(bytes, bytes + QUEST_MOBILE_NATIVE_REFERENCE_BYTES,
					[](uint8_t value) { return value == 0; }))
				continue;
			quest_mobile_native_reference reference;
			if (quest_mobile_native_reference_decode(
				    { bytes, QUEST_MOBILE_NATIVE_REFERENCE_BYTES }, &reference) !=
				    player_snapshot_codec_result::ok ||
			    reference.mobile_instance_id == b.reference.mobile_instance_id ||
			    reference.birth_operation.bytes == b.reference.birth_operation.bytes)
				return false;
		}
		size_t retained_rows = 0;
		for (const auto &item : b.stock)
			if (item.stage)
			{
				++retained_rows;
				if (item.published || item.enrolled || !item.object ||
				    item.stage->object() != item.object ||
				    item.object->obj_uid != item.uid)
					return false;
				for (P_obj obj = object_list; obj; obj = obj->next)
					if (obj == item.object || obj->obj_uid == item.uid)
						return false;
				item_ownership_runtime_entry occupied{};
				if (item_ownership_runtime_lookup(item.uid, &occupied))
					return false;
			}
		if (retained_rows != b.image.items.size())
			return false;
		// All allocating source/world observations precede the final real pin check.
		return request.worker_->borrowed_current();
	}
	catch (...)
	{
		return false;
	}
}

void quest_mobile_native_birth_owner::service_ordinary_flat_execution_requests() noexcept
{
	if (!nevent_is_game_thread())
		return;
	for (;;)
	{
		quest_mobile_native_birth_ordinary_execution_lease *request = nullptr;
		try
		{
			{
				std::lock_guard<std::mutex> lock(ordinary_birth_request_mutex);
				for (auto *item = ordinary_birth_request_head; item;
				     item = item->next_)
					if (item->phase_ ==
					    quest_mobile_native_birth_ordinary_execution_lease::
						    phase::requested)
					{
						request = item;
						request->game_holds_request_ = true;
						request->phase_ =
							quest_mobile_native_birth_ordinary_execution_lease::
								phase::inspecting;
						break;
					}
			}
			if (!request)
				return;
			original_birth *producer = nullptr;
			// Exact registry owner plus full body proof; ID matching alone grants nothing.
			for (const auto &entry : births)
				if (entry && entry->ordinary_flat_source && request->worker_ &&
				    entry->reference.birth_operation.bytes ==
					    request->worker_->command().operation_id.bytes)
				{
					if (producer)
					{
						producer = nullptr;
						break;
					}
					producer = entry.get();
				}
			const bool valid = producer && ordinary_flat_execution_source_current(
							       *request, producer);
			std::unique_lock<std::mutex> lock(ordinary_birth_request_mutex);
			if (valid && request->phase_ ==
					     quest_mobile_native_birth_ordinary_execution_lease::
						     phase::inspecting)
			{
				request->producer_ = producer;
				request->source_ = producer->ordinary_flat_source.get();
				request->phase_ =
					quest_mobile_native_birth_ordinary_execution_lease::phase::
						granted;
				ordinary_birth_request_changed.notify_all();
				// The bound main game thread performs no world mutation while
				// waiting; wait releases request mutex. No coordinator/pipeline/
				// identity/authority/SQL lock is held. Worker reads no live world.
				ordinary_birth_request_changed.wait(
					lock,
					[&]() {
						return request->phase_ ==
						       quest_mobile_native_birth_ordinary_execution_lease::
							       phase::released;
					});
			}
			else if (request->phase_ !=
				 quest_mobile_native_birth_ordinary_execution_lease::phase::released)
				request->phase_ =
					quest_mobile_native_birth_ordinary_execution_lease::phase::
						refused;
			request->game_holds_request_ = false;
			ordinary_birth_request_changed.notify_all();
		}
		catch (...)
		{
			// Never continue with an unacknowledged cross-thread stack borrow.
			std::terminate();
		}
	}
}

// Unselected original M factory/source prerequisite only. General flat M/G/E
// dispatch, genuine NPC item provenance/binding, command freeze and publication
// remain separate primary-owned joins. This is not a ROOM source reinterpretation.
bool quest_mobile_native_birth_owner::flat_mobile_factory_current(int rnum, int room, uint32_t slot,
								  size_t private_live) noexcept
{
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY || !reset_in_progress ||
	    !replay_ready || !reset_dispatch.flat_backend || !reset_dispatch.valid ||
	    !reset_dispatch.open || reset_dispatch.completed || reset_dispatch.aborted ||
	    reset_dispatch.current_command != 'M' || reset_dispatch.current_slot != slot ||
	    !reset_dispatch.locals.command_entered || reset_dispatch.locals.cmd_no < 0 ||
	    static_cast<uint32_t>(reset_dispatch.locals.cmd_no) != slot || !zone_table ||
	    reset_zone_rnum < 0 || reset_zone_rnum > top_of_zone_table ||
	    !reset_dispatch.commands ||
	    zone_table[reset_zone_rnum].cmd != reset_dispatch.commands ||
	    zone_table[reset_zone_rnum].number != reset_dispatch.zone_vnum || !world || room < 0 ||
	    room > top_of_world || !mob_index || rnum < 0 || rnum > top_of_mobt)
		return false;
	const auto &actual = reset_dispatch.commands[slot];
	const auto &captured = reset_dispatch.original_command;
	if (actual.command != 'M' || actual.arg1 != rnum || actual.arg3 != room ||
	    actual.command != captured.command || actual.if_flag != captured.if_flag ||
	    actual.arg1 != captured.arg1 || actual.arg2 != captured.arg2 ||
	    actual.arg3 != captured.arg3 || actual.arg4 != captured.arg4)
		return false;
	const char *configured = persistence_mode_flatfile_root();
	if (!configured || reset_dispatch.selected_flat_root.empty() ||
	    reset_dispatch.selected_flat_root != configured)
		return false;
	// Full genuine same projection observation as reset_flat_projection_current,
	// with the real still-private birth/source caller lifetime supplied by this
	// private owning constructor. Both actual IDs die before the refresh.
	const size_t own_frames =
		sizeof(rnum) + sizeof(room) + sizeof(slot) + sizeof(private_live) +
		sizeof(&actual) + sizeof(&captured) + sizeof(configured) + sizeof(size_t) +
		sizeof(size_t) + sizeof(size_t) + // own_frames/surviving/scratch
		sizeof(bool) + sizeof(bool) + sizeof(bool) + // matches/recensused/return
		npc_flat_projection_source_frames();
	size_t surviving = private_live;
	if (!add_bytes(surviving, own_frames))
		return false;
	size_t scratch = surviving;
	if (!add_bytes(scratch, sizeof(critical_operation_id)) ||
	    !add_bytes(scratch, sizeof(critical_operation_id)) || !charge(scratch))
		return false;
	bool matches = false;
	{
		critical_operation_id lineage{}, epoch{};
		matches = economic_gameplay_authority::capture_flat_reset_projection(&lineage,
										     &epoch) &&
			  lineage.bytes == reset_dispatch.flat_lineage.bytes &&
			  epoch.bytes == reset_dispatch.flat_epoch.bytes;
	}
	const bool recensused = charge(surviving);
	return matches && recensused;
}

bool quest_mobile_native_birth_owner::capture_flat_mobile_factory_source(
	int rnum, int room, uint32_t slot, economic_source_event *source, int32_t *zone_vnum,
	size_t private_live) noexcept
{
	if (!source || !zone_vnum)
		return false;
	constexpr size_t own_frames =
		sizeof(rnum) + sizeof(room) + sizeof(slot) + sizeof(source) + sizeof(zone_vnum) +
		sizeof(private_live) + sizeof(size_t) + sizeof(bool) +
		sizeof(economic_source_event); // live/return/actual source aggregate
	size_t live = private_live;
	if (!add_bytes(live, own_frames) || !flat_mobile_factory_current(rnum, room, slot, live))
		return false;
	// Preserve the original lazy nonce cut: the caller already sealed the
	// previous birth, allocated its private owner and reserved births capacity.
	// Original nonce-attempt latch, generator and transfer are unchanged.
	if (critical_operation_id_is_zero(reset_invocation))
	{
		if (reset_dispatch.invocation_attempted)
			return false;
		size_t request = live;
		if (!add_bytes(request, sizeof(request)) ||
		    !add_bytes(request, sizeof(critical_operation_id)) || !charge(request))
			return false;
		reset_dispatch.invocation_attempted = true;
		critical_operation_id invocation{};
		if (!critical_operation_id_generate(&invocation))
			return false;
		reset_invocation = invocation;
	}
	// These are private original value outputs, not factory/source authority.
	// Only the final genuine sealed producer may mint the separate closed pin.
	*source = { economic_source_kind::npc_generation, reset_invocation, reset_invocation, 0,
		    slot };
	*zone_vnum = reset_dispatch.zone_vnum;
	return true;
}

P_char quest_mobile_native_birth_owner::prepare_mobile_flat(int rnum, int room, uint32_t slot,
							    int shop) noexcept
{
	if (!reset_in_progress || !replay_ready || !nevent_is_game_thread() || room < 0 ||
	    room > top_of_world || rnum < 0 || rnum > top_of_mobt || !mob_index || !world)
		return nullptr;
	// Only the actual entered/open original M frame may construct this body.
	// No caller DTO, replay image or source pin can open that frame.
	if (!flat_mobile_factory_current(rnum, room, slot,
					 sizeof(rnum) + sizeof(room) + sizeof(slot) + sizeof(shop) +
						 sizeof(P_char)))
		return nullptr;
	seal_mobile_flat(sizeof(rnum) + sizeof(room) + sizeof(slot) + sizeof(shop) +
			 sizeof(P_char));
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
		economic_source_event original_source{};
		int32_t original_zone_vnum = -1;
		// The fresh private birth has no dynamic payload yet. Its actual
		// allocation is not in births until the original insertion below.
		// Carry it, its unique_ptr and the actual source/output/scalar locals
		// through the full projection/nonce proof; never use bare charge(0)
		// while this genuine private allocation is alive.
		size_t private_live = sizeof(*b) + sizeof(b) + sizeof(available) +
				      sizeof(original_source) + sizeof(original_zone_vnum) +
				      sizeof(size_t) + 3 * sizeof(int) + sizeof(uint32_t) +
				      sizeof(P_char) + sizeof(void *);
		if (!capture_flat_mobile_factory_source(rnum, room, slot, &original_source,
							&original_zone_vnum, private_live))
			return nullptr;
		auto &ref = b->reference;
		if (!critical_operation_id_generate(&ref.birth_operation) ||
		    ref.birth_operation.bytes == reset_invocation.bytes)
			return nullptr;
		ref.mobile_instance_id = item_uid_allocator_next();
		if (!ref.mobile_instance_id || ref.mobile_instance_id == UINT64_MAX)
			return nullptr;
		ref.birth_source = original_source;
		ref.mobile_vnum = mob_index[rnum].virtual_number;
		ref.birthplace_vnum = world[room].number;
		ref.reset_zone_vnum = original_zone_vnum;
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
		// Full genuine original caller survives the registry transfer. The birth
		// allocation and accepted metadata now belong to CURRENT exactly once.
		size_t mint_outer = sizeof(rnum) + sizeof(room) + sizeof(slot) + sizeof(shop) +
				    sizeof(P_char) + sizeof(available) + sizeof(b) +
				    sizeof(original_source) + sizeof(original_zone_vnum) +
				    sizeof(private_live) + sizeof(&ref) + sizeof(running_build) +
				    sizeof(index) + sizeof(size_t);
		if (!charge(mint_outer))
		{
			births[index]->blocked = true;
			current_birth = SIZE_MAX;
			discard(index);
			return nullptr;
		}
		if (births[index]->blocked)
			return nullptr;
		if (!capture_npc_flat_factory_scope(index, mint_outer))
		{
			births[index]->blocked = true;
			current_birth = SIZE_MAX;
			discard(index);
			return nullptr;
		}
		return births[index]->character;
	}
	catch (...)
	{
		return nullptr;
	}
}

size_t quest_mobile_native_birth_owner::npc_flat_projection_source_frames() noexcept
{
	// Exact capture_flat_reset_projection source, including selected and the
	// optional distinct atomic-load return carrier, with actual load/lock locals.
	constexpr size_t capture = sizeof(critical_operation_id *) +
				   sizeof(critical_operation_id *) + sizeof(const char *) +
				   sizeof(bool);
	constexpr size_t atomic_load =
		sizeof(const void *) + sizeof(std::memory_order) + sizeof(const void *) +
		sizeof(std::memory_order) + sizeof(void *) + sizeof(const void *) +
		sizeof(std::memory_order) + sizeof(uintptr_t) + sizeof(void *) + sizeof(void *) +
		sizeof(void *) + sizeof(const void *) + sizeof(std::memory_order) +
		sizeof(const void *) + sizeof(int);
	constexpr size_t is_zero = sizeof(const critical_operation_id *) + sizeof(uint8_t) +
				   sizeof(const uint8_t *) + sizeof(const uint8_t *) + sizeof(bool);
	return capture + atomic_load + is_zero + sizeof(size_t) +
	       economic_gameplay_authority::active_regular_flat_working_bytes() +
	       economic_gameplay_authority::active_regular_flat_working_bytes();
}

size_t quest_mobile_native_birth_owner::npc_flat_factory_scope_current_frames() noexcept
{
	// scope argument, configured root, actual owner, actual loop entry/begin/end,
	// birth and original-M references, genuine projection output carriers.
	constexpr size_t observer =
		sizeof(const quest_mobile_native_npc_flat_factory_scope *) + sizeof(const char *) +
		sizeof(const original_birth *) + sizeof(const std::unique_ptr<original_birth> *) +
		sizeof(decltype(births.cbegin())) + sizeof(decltype(births.cend())) +
		sizeof(const original_birth *) + sizeof(decltype(&zone_table[0].cmd[0])) +
		sizeof(critical_operation_id) + sizeof(critical_operation_id) + sizeof(bool);
	// same_owner parameters/return plus source-level equal array/string comparison
	// carriers. No container heap is allocated by these identity comparisons.
	constexpr size_t comparison =
		sizeof(const quest_mobile_native_npc_flat_factory_scope *) +
		sizeof(const quest_mobile_native_npc_flat_factory_scope *) + sizeof(bool) +
		sizeof(const std::string *) + sizeof(const std::string *) + sizeof(const char *) +
		sizeof(const char *) + sizeof(size_t) + sizeof(int) +
		sizeof(const std::array<uint8_t, 16> *) + sizeof(const std::array<uint8_t, 16> *) +
		sizeof(const std::array<uint8_t, 32> *) + sizeof(const std::array<uint8_t, 32> *) +
		sizeof(const std::array<int, 4> *) + sizeof(const std::array<int, 4> *) +
		sizeof(const uint8_t *) + sizeof(const uint8_t *) + sizeof(const uint8_t *) +
		sizeof(const int *) + sizeof(const int *) + sizeof(const int *) +
		sizeof(std::ptrdiff_t) + sizeof(bool);
	// Actual capture_flat_reset_projection parameters, root local and return;
	// real selected shared_ptr carrier is supplied by its existing provider.
	constexpr size_t projection = sizeof(critical_operation_id *) +
				      sizeof(critical_operation_id *) + sizeof(const char *) +
				      sizeof(bool) + sizeof(std::memory_order) + sizeof(size_t);
	// Actual fixed native identity helper carriers. The map lookup allocates
	// nothing: wrapper/key/iterator and actual node/hash/bucket traversals.
	constexpr size_t identity_lookup =
		sizeof(uint64_t) + sizeof(std::unordered_map<uint64_t, P_char>::iterator) +
		sizeof(P_char) + sizeof(const char *) + sizeof(bool) + // nevent_require_game_thread
		sizeof(std::unordered_map<uint64_t, P_char> *) + sizeof(const uint64_t *) +
		sizeof(std::unordered_map<uint64_t, P_char>::iterator) + sizeof(size_t) +
		sizeof(size_t) + // hash code / bucket
		sizeof(const void *) + sizeof(size_t) + sizeof(const uint64_t *) + sizeof(size_t) +
		sizeof(void *) + // find_node before node
		sizeof(const void *) + sizeof(size_t) + sizeof(const uint64_t *) + sizeof(size_t) +
		sizeof(void *) + sizeof(void *) + // find_before_node prev/node
		sizeof(const void *) + sizeof(const uint64_t *) + sizeof(size_t) +
		sizeof(const void *) + sizeof(const uint64_t *) + sizeof(const uint64_t *) +
		sizeof(bool); // key equality
	// Actual atomic shared_ptr load scopes: public load, _Sp_atomic::load,
	// count lock/unlock and _S_add_ref. Shared_ptr returned carrier may coexist
	// with selected; the actual shared_ptr object size comes from its owner.
	constexpr size_t atomic_load =
		sizeof(const void *) + sizeof(std::memory_order) + sizeof(const void *) +
		sizeof(std::memory_order) + sizeof(void *) + sizeof(const void *) +
		sizeof(std::memory_order) + sizeof(uintptr_t) + sizeof(void *) + sizeof(void *) +
		sizeof(void *) + // _S_add_ref argument/return
		sizeof(const void *) + sizeof(std::memory_order) + // count unlock
		sizeof(const void *) + sizeof(int) + // _M_add_ref_copy / increment
		sizeof(const critical_operation_id *) + sizeof(uint8_t) + sizeof(const uint8_t *) +
		sizeof(const uint8_t *) + sizeof(bool); // ID is_zero
	return observer + comparison + projection + identity_lookup + atomic_load +
	       economic_gameplay_authority::active_regular_flat_working_bytes() +
	       economic_gameplay_authority::active_regular_flat_working_bytes();
}

bool quest_mobile_native_birth_owner::npc_flat_factory_scope_current(
	const quest_mobile_native_npc_flat_factory_scope &scope) noexcept
{
	const char *configured = persistence_mode_flatfile_root();
	if (!nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY || !configured ||
	    scope.root_.empty() || scope.root_ != configured || !scope.owner_ ||
	    scope.source_.kind != economic_source_kind::npc_generation || scope.source_.sequence ||
	    critical_operation_id_is_zero(scope.source_.source) ||
	    scope.source_.source.bytes != scope.source_.generation.bytes ||
	    critical_operation_id_is_zero(scope.operation_) || !scope.native_id_ ||
	    scope.native_id_ == UINT64_MAX || !scope.runtime_id_)
		return false;
	const original_birth *actual = nullptr;
	for (const auto &entry : births)
		if (entry && entry.get() == scope.owner_)
		{
			actual = entry.get();
			break;
		}
	if (!actual || !actual->npc_flat_factory_scope ||
	    !actual->npc_flat_factory_scope->same_owner(scope))
		return false;
	const auto &b = *actual;
	if (!zone_table || scope.zone_ < 0 || scope.zone_ > top_of_zone_table || !scope.commands_ ||
	    zone_table[scope.zone_].cmd != scope.commands_ ||
	    zone_table[scope.zone_].number != scope.zone_vnum_)
		return false;
	const auto &original_m = zone_table[scope.zone_].cmd[scope.source_.slot];
	if (original_m.command != 'M' || original_m.if_flag != scope.original_if_flag_ ||
	    original_m.arg1 != scope.original_m_args_[0] ||
	    original_m.arg2 != scope.original_m_args_[1] ||
	    original_m.arg3 != scope.original_m_args_[2] ||
	    original_m.arg4 != scope.original_m_args_[3])
		return false;
	if (b.cold || b.blocked || b.retired || b.mobile_consumed || b.mobile_started ||
	    !b.constructor_present || !b.character || b.character != scope.character_ ||
	    b.mobile.character() != b.character || b.runtime_id != scope.runtime_id_ ||
	    b.character->runtime_id != scope.runtime_id_ || b.character->in_room != NOWHERE ||
	    find_character_by_runtime_id(scope.runtime_id_) ||
	    b.reference.mobile_instance_id != scope.native_id_ ||
	    b.reference.birth_operation.bytes != scope.operation_.bytes ||
	    b.reference.birth_source.kind != scope.source_.kind ||
	    b.reference.birth_source.source.bytes != scope.source_.source.bytes ||
	    b.reference.birth_source.generation.bytes != scope.source_.generation.bytes ||
	    b.reference.birth_source.sequence != scope.source_.sequence ||
	    b.reference.birth_source.slot != scope.source_.slot || b.zone != scope.zone_ ||
	    b.reference.reset_zone_vnum != scope.zone_vnum_ || b.room != scope.room_ ||
	    b.rnum != scope.rnum_ || b.shop != scope.shop_ ||
	    b.constructor.build_digest != scope.build_ ||
	    b.constructor.procedure_before != scope.procedure_before_ ||
	    b.constructor.procedure_after != scope.procedure_after_ ||
	    b.constructor.reset_tail != scope.reset_tail_)
		return false;
	critical_operation_id lineage{}, epoch{};
	return economic_gameplay_authority::capture_flat_reset_projection(&lineage, &epoch) &&
	       lineage.bytes == scope.lineage_.bytes && epoch.bytes == scope.epoch_.bytes;
}
bool quest_mobile_native_birth_owner::capture_npc_flat_factory_scope(size_t index,
								     size_t outer_live) noexcept
{
	if (index >= births.size() || !births[index] || births[index]->npc_flat_factory_scope)
		return false;
	auto &b = *births[index];
	if (current_birth != index || b.cold || b.blocked || b.sealed || b.submitted ||
	    b.mobile_started || b.mobile_consumed || !b.constructor_present || !b.character ||
	    b.mobile.character() != b.character || b.character->runtime_id != b.runtime_id ||
	    b.reference.birth_source.kind != economic_source_kind::npc_generation ||
	    b.reference.birth_source.sequence ||
	    b.reference.birth_source.source.bytes != reset_invocation.bytes ||
	    b.reference.birth_source.generation.bytes != reset_invocation.bytes ||
	    !flat_mobile_factory_current(b.rnum, b.room, b.reference.birth_source.slot,
					 outer_live > SIZE_MAX - (sizeof(index) +
								  sizeof(outer_live) + sizeof(&b) +
								  sizeof(bool)) ?
						 SIZE_MAX :
						 outer_live + sizeof(index) + sizeof(outer_live) +
							 sizeof(&b) + sizeof(bool)))
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
	size_t request =
		sizeof(quest_mobile_native_npc_flat_factory_scope) +
		sizeof(std::_Sp_counted_ptr<quest_mobile_native_npc_flat_factory_scope *,
					    __gnu_cxx::_S_atomic>) +
		sizeof(std::shared_ptr<const quest_mobile_native_npc_flat_factory_scope>) +
		sizeof(index) + sizeof(outer_live) + sizeof(&b) + sizeof(size_t) + sizeof(bool) +
		sizeof(std::array<int, 4>) +
		// Real original constructor argument carriers coexist with root copy.
		sizeof(const std::string *) + sizeof(const void *) + sizeof(P_char) +
		sizeof(uint64_t) + sizeof(uint64_t) + sizeof(const critical_operation_id *) +
		sizeof(const economic_source_event *) + sizeof(const critical_operation_id *) +
		sizeof(const critical_operation_id *) + sizeof(int) + sizeof(int) + sizeof(int) +
		sizeof(int) + sizeof(int) + sizeof(const void *) +
		sizeof(const std::array<int, 4> *) + sizeof(bool) +
		sizeof(const quest_mobile_native_constructor_recipe *) +
		quest_mobile_native_npc_flat_factory_scope::copy_source_frames();
	if (reset_dispatch.selected_flat_root.size() > 15)
	{
		if (reset_dispatch.selected_flat_root.size() == SIZE_MAX ||
		    !add_bytes(request, reset_dispatch.selected_flat_root.size() + 1))
			return false;
	}
	if (!add_bytes(request, outer_live) || !charge(request))
		return false;
	try
	{
		b.npc_flat_factory_scope =
			std::shared_ptr<const quest_mobile_native_npc_flat_factory_scope>(
				new quest_mobile_native_npc_flat_factory_scope(
					reset_dispatch.selected_flat_root, &b, b.character,
					b.runtime_id, b.reference.mobile_instance_id,
					b.reference.birth_operation, b.reference.birth_source,
					reset_dispatch.flat_lineage, reset_dispatch.flat_epoch,
					b.zone, b.reference.reset_zone_vnum, b.room, b.rnum, b.shop,
					reset_dispatch.commands,
					{ reset_dispatch.original_command.arg1,
					  reset_dispatch.original_command.arg2,
					  reset_dispatch.original_command.arg3,
					  reset_dispatch.original_command.arg4 },
					reset_dispatch.original_command.if_flag, b.constructor));
		// request's heap/constructor terms have transferred into CURRENT; only
		// live caller/mint scalars remain private for this new observation.
		size_t current_outer = outer_live;
		if (!add_bytes(current_outer, sizeof(index)) ||
		    !add_bytes(current_outer, sizeof(outer_live)) ||
		    !add_bytes(current_outer, sizeof(&b)) ||
		    !add_bytes(current_outer, sizeof(request)) ||
		    !add_bytes(current_outer, sizeof(current_outer)) ||
		    !b.npc_flat_factory_scope->current_bounded(reserve_npc_binding_scratch, nullptr,
							       current_outer))
		{
			b.npc_flat_factory_scope.reset();
			charge(current_outer);
			return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
#else
	return false;
#endif
}
bool quest_mobile_native_birth_owner::borrow_npc_flat_factory_scope(
	P_char actor, const quest_mobile_native_npc_flat_factory_scope **output,
	size_t outer_live) noexcept
{
	if (!output || !actor || !owns(actor) || !nevent_is_game_thread())
		return false;
	const auto &b = *births[current_birth];
	size_t current_outer = outer_live;
	if (!add_bytes(current_outer, sizeof(actor)) || !add_bytes(current_outer, sizeof(output)) ||
	    !add_bytes(current_outer, sizeof(outer_live)) ||
	    !add_bytes(current_outer, sizeof(&b)) ||
	    !add_bytes(current_outer, sizeof(current_outer)) ||
	    !add_bytes(current_outer, sizeof(bool)) || !b.npc_flat_factory_scope ||
	    !b.npc_flat_factory_scope->current_bounded(reserve_npc_binding_scratch, nullptr,
						       current_outer))
		return false;
	// Borrowed solely from the actual retained constructor owner. Its immutable
	// storage ends with that owner; every consumer reauthenticates current().
	*output = b.npc_flat_factory_scope.get();
	return true;
}
bool quest_mobile_native_birth_owner::reserve_npc_binding_scratch(size_t live,
								  void *context) noexcept
{
	// Candidate/map/UID set/request storage is supplied by the real DB owner.
	// Current birth registry, sibling stages and all other retained state are
	// observed by charge; the incoming private candidate never duplicates it.
	return add_bytes(live, sizeof(live)) && add_bytes(live, sizeof(context)) &&
	       add_bytes(live, sizeof(bool)) && charge(live);
}

P_obj quest_mobile_native_birth_owner::prepare_item_flat(int rnum, size_t outer_live) noexcept
{
	if (current_birth >= births.size() || !births[current_birth] ||
	    births[current_birth]->blocked || rnum < 0 || rnum > top_of_objt)
		return nullptr;
	auto &b = *births[current_birth];
	size_t entry_outer = outer_live;
	if (!add_bytes(entry_outer, sizeof(rnum)) || !add_bytes(entry_outer, sizeof(outer_live)) ||
	    !add_bytes(entry_outer, sizeof(&b)) || !add_bytes(entry_outer, sizeof(entry_outer)) ||
	    !add_bytes(entry_outer, sizeof(P_obj)) || !b.npc_flat_factory_scope ||
	    !b.npc_flat_factory_scope->current_bounded(reserve_npc_binding_scratch, nullptr,
						       entry_outer))
		return nullptr;
	original_item item;
	struct scope_copy_context
	{
		quest_mobile_native_item_stage *stage;
		size_t private_live;
	} scratch{ nullptr, 0 };
	const auto reserve_scope = [](size_t extra, void *opaque) noexcept
	{
		const auto &state = *static_cast<const scope_copy_context *>(opaque);
		size_t bytes = state.private_live;
		size_t native = sizeof(quest_mobile_native_item_stage);
		if (state.stage && !state.stage->empty())
		{
			if (item_native_quest_global_budget_scope_owner::literal_pool_owned() &&
			    state.stage->is_flat_factory())
			{
				if (!state.stage->retained_bytes_excluding_literal_pools(&native))
					return false;
			}
			else
			{
				native = state.stage->retained_bytes();
				if (!native)
					return false;
			}
		}
		// Callback extra/opaque/state/bytes/native/return and the genuinely
		// selected observer's complete declared source carriers coexist here.
		return add_bytes(bytes, native) && add_bytes(bytes, extra) &&
		       add_bytes(bytes, sizeof(extra)) && add_bytes(bytes, sizeof(opaque)) &&
		       add_bytes(bytes, sizeof(&state)) && add_bytes(bytes, sizeof(bytes)) &&
		       add_bytes(bytes, sizeof(native)) && add_bytes(bytes, sizeof(bool)) &&
		       add_bytes(bytes, sizeof(const quest_mobile_native_item_stage
						       *)) && // empty/is_flat_factory this
		       add_bytes(bytes, sizeof(bool)) && // leaf return
		       add_bytes(bytes, sizeof(size_t *)) &&
		       add_bytes(bytes, sizeof(size_t)) &&
		       add_bytes(bytes, sizeof(bool)) && // add_bytes reference/amount/return
		       add_bytes(bytes, quest_mobile_native_item_stage::
						npc_retained_observation_source_frames()) &&
		       charge(bytes);
	};
	scratch.private_live = outer_live;
	if (!add_bytes(scratch.private_live, sizeof(item)) ||
	    !add_bytes(scratch.private_live, sizeof(scratch)) ||
	    !add_bytes(scratch.private_live, sizeof(reserve_scope)) ||
	    !add_bytes(scratch.private_live, sizeof(rnum)) ||
	    !add_bytes(scratch.private_live, sizeof(outer_live)) ||
	    !add_bytes(scratch.private_live, sizeof(&b)) ||
	    !add_bytes(scratch.private_live, sizeof(entry_outer)) ||
	    !add_bytes(scratch.private_live, sizeof(P_obj)) ||
	    !add_bytes(scratch.private_live, sizeof(size_t))) // actual request local
		return nullptr;
	try
	{
		if (b.stock.size() >= PLAYER_SNAPSHOT_MAX_ROWS)
		{
			b.blocked = true;
			return nullptr;
		}
		size_t request = scratch.private_live;
		if (!add_bytes(request, sizeof(quest_mobile_native_item_stage)) ||
		    !add_bytes(request, sizeof(std::unique_ptr<quest_mobile_native_item_stage>)) ||
		    !charge(request))
		{
			b.blocked = true;
			return nullptr;
		}
		item.stage = std::make_unique<quest_mobile_native_item_stage>();
		scratch.stage = item.stage.get();
		request = 0;
		if (b.stock.size() == b.stock.capacity())
		{
			if (b.stock.size() == SIZE_MAX ||
			    b.stock.size() + 1 > SIZE_MAX / sizeof(original_item))
			{
				b.blocked = true;
				return nullptr;
			}
			request = (b.stock.size() + 1) * sizeof(original_item);
		}
		if (!reserve_scope(request, &scratch))
		{
			b.blocked = true;
			return nullptr;
		}
		b.stock.reserve(b.stock.size() + 1);
		item.uid = item_uid_allocator_next();
		item.rnum = rnum;
		if (!item.uid || !quest_mobile_native_item_stage::prepare_retaining_npc_flat(
					 rnum, REAL, item.uid, *b.npc_flat_factory_scope,
					 item.stage.get(), reserve_scope, &scratch))
		{
			// Genuine unresolved native state cannot disappear with the local
			// stage pointer. The already-reserved stock slot retains it closed.
			item.object = item.stage->object();
			b.stock.push_back(std::move(item));
			b.blocked = true;
			charge(scratch.private_live);
			return nullptr;
		}
		item.object = item.stage->object();
		const size_t steps = item.stage->publication_step_count();
		if (!add_bytes(scratch.private_live, sizeof(steps)))
		{
			b.stock.push_back(std::move(item));
			b.blocked = true;
			charge(scratch.private_live);
			return nullptr;
		}
		if (steps > SIZE_MAX / sizeof(quest_mobile_native_item_effect) ||
		    !reserve_scope(steps * sizeof(quest_mobile_native_item_effect), &scratch))
		{
			b.stock.push_back(std::move(item));
			b.blocked = true;
			charge(scratch.private_live);
			return nullptr;
		}
		item.effects.resize(steps);
		P_obj result = item.object;
		// The actual effects/native stage move into registry storage; the local
		// object and result carrier remain alive, and no private heap is duplicated.
		if (!add_bytes(scratch.private_live, sizeof(result)))
		{
			b.stock.push_back(std::move(item));
			b.blocked = true;
			charge(scratch.private_live);
			return nullptr;
		}
		b.stock.push_back(std::move(item));
		if (!charge(scratch.private_live))
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
		if (item.stage && !item.stage->discard_unadmitted())
		{
			// Original reserve succeeded before any native construction. The
			// fitting move cannot allocate; retain only genuinely surviving state.
			item.object = item.stage->object();
			if (b.stock.size() < b.stock.capacity())
				b.stock.push_back(std::move(item));
		}
		b.blocked = true;
		charge(scratch.private_live);
		return nullptr;
	}
}

bool quest_mobile_native_birth_owner::capture_alchemist_spawn_flat(P_char actor, int room) noexcept
{
	if (!reset_in_progress || !nevent_is_game_thread() || !owns(actor) ||
	    room != births[current_birth]->room || room <= NOWHERE || room > top_of_world)
		return false;
	auto &birth = *births[current_birth];
	if (!birth.npc_flat_factory_scope ||
	    !birth.npc_flat_factory_scope->current_bounded(reserve_npc_binding_scratch, nullptr,
							   sizeof(actor) + sizeof(room) +
								   sizeof(&birth) + sizeof(bool)))
		return false;
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
	const size_t item_outer = sizeof(actor) + sizeof(room) + sizeof(&birth) + sizeof(choice) +
				  sizeof(rnum) + sizeof(P_obj) + sizeof(bool) + sizeof(size_t);
	P_obj vial = prepare_item_flat(rnum, item_outer);
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
void quest_mobile_native_birth_owner::seal_mobile_flat(size_t outer_live) noexcept
{
	if (current_birth >= births.size() || !births[current_birth])
		return;
	const size_t index = current_birth;
	auto &b = *births[index];
	current_birth = SIZE_MAX;
	if (b.blocked || b.sealed || !b.character)
		return;
	if (!b.npc_flat_factory_scope ||
	    !b.npc_flat_factory_scope->current_bounded(
		    reserve_npc_binding_scratch, nullptr,
		    outer_live > SIZE_MAX - (sizeof(outer_live) + sizeof(index) + sizeof(&b)) ?
			    SIZE_MAX :
			    outer_live + sizeof(outer_live) + sizeof(index) + sizeof(&b)))
	{
		b.blocked = true;
		return;
	}
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
		size_t binding_outer = sizeof(outer_live) + sizeof(inputs) + sizeof(index) +
				       sizeof(&b) + sizeof(selected) +
				       sizeof(size_t) + // actual binding_outer local
				       sizeof(std::span<const quest_mobile_native_item_binding>);
		if (!add_bytes(binding_outer, outer_live) ||
		    inputs.capacity() >
			    (SIZE_MAX - binding_outer) / sizeof(quest_mobile_native_item_binding))
		{
			b.blocked = true;
			return;
		}
		binding_outer += inputs.capacity() * sizeof(quest_mobile_native_item_binding);
		const std::span<const quest_mobile_native_item_binding> originals(inputs);
		if (!shop_trade_original_procedure_binding_stage::
			    prepare_native_birth_npc_flat_bounded(
				    originals, *b.npc_flat_factory_scope, b.bindings,
				    reserve_npc_binding_scratch, nullptr, binding_outer))
		{
			b.blocked = true;
			return;
		}
		b.sealed = true;
		if (b.cash_role.role == native_mobile_birth_cash_role::ordinary_wallet &&
		    reset_dispatch.flat_backend && !capture_ordinary_flat_source_pin(index))
		{
			b.blocked = true;
			return;
		}
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
void quest_mobile_native_birth_owner::block_mobile_flat() noexcept
{
	if (current_birth < births.size() && births[current_birth])
		births[current_birth]->blocked = true;
	seal_mobile_flat(0);
}
void quest_mobile_native_birth_owner::finish_reset_flat() noexcept
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
	seal_mobile_flat(0);
	reset_in_progress = false;
	reset_zone_rnum = -1;
	reset_invocation = {};
	reset_dispatch = {};
	charge();
}

namespace
{
bool birth_ordinary_flat_role_encode_bounded(const native_mobile_birth_cash_role_recipe &value,
					     native_mobile_birth_cash_role_recipe_bytes *output,
					     bool (*reserve)(size_t, void *) noexcept,
					     void *context, size_t outer) noexcept
{
	if (!output || !native_mobile_birth_cash_role_recipe_valid(value))
		return false;
	size_t base = outer;
	constexpr size_t frames =
		sizeof(&value) + sizeof(output) + sizeof(reserve) + sizeof(context) +
		sizeof(outer) + sizeof(base) + sizeof(bool) + sizeof(std::vector<uint8_t>) +
		sizeof(native_mobile_birth_cash_role_recipe_bytes) + sizeof(size_t);
	if (!birth_passive_add(base, frames) || !reserve || !reserve(base, context))
		return false;
	try
	{
		std::vector<uint8_t> original;
		if (!native_mobile_birth_constructor_recipe_encode_blob_bounded(
			    value.original, &original, reserve, context, base) ||
		    original.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES)
			return false;
		native_mobile_birth_cash_role_recipe_bytes candidate{};
		candidate[0] = 'N';
		candidate[1] = 'B';
		candidate[2] = 'C';
		candidate[3] = '4';
		candidate[4] = NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION;
		size_t copy_live = base;
		using input_iterator = decltype(original.begin());
		using output_iterator = decltype(candidate.begin());
		// Actual installed copy / __copy_move_a / __copy_move_a1 /
		// __copy_move_a2 / bulk __copy_m parameters and return carriers.
		// Normal-iterator begin/end, miter/niter base and final wrap are real.
		constexpr size_t copy_frames =
			sizeof(copy_live) + sizeof(std::vector<uint8_t> *) +
			sizeof(input_iterator) + sizeof(std::vector<uint8_t> *) +
			sizeof(input_iterator) +
			sizeof(native_mobile_birth_cash_role_recipe_bytes *) +
			sizeof(output_iterator) + sizeof(output_iterator) + sizeof(std::ptrdiff_t) +
			sizeof(output_iterator) + sizeof(input_iterator) + sizeof(input_iterator) +
			sizeof(output_iterator) + sizeof(output_iterator) + sizeof(input_iterator) +
			sizeof(input_iterator) + sizeof(output_iterator) + sizeof(output_iterator) +
			sizeof(input_iterator) + sizeof(input_iterator) + sizeof(input_iterator) +
			sizeof(input_iterator) + sizeof(const input_iterator *) +
			sizeof(uint8_t *) + sizeof(const input_iterator *) + sizeof(uint8_t *) +
			sizeof(output_iterator) + sizeof(output_iterator) + sizeof(uint8_t *) +
			sizeof(uint8_t *) + sizeof(output_iterator) + sizeof(output_iterator) +
			sizeof(uint8_t *) + sizeof(uint8_t *) + sizeof(output_iterator) +
			sizeof(output_iterator) + sizeof(uint8_t *) + sizeof(uint8_t *) +
			sizeof(output_iterator) + sizeof(std::ptrdiff_t) + sizeof(output_iterator) +
			sizeof(void *) + sizeof(const void *) + sizeof(size_t) + sizeof(void *) +
			sizeof(bool);
		if (!birth_passive_add(copy_live, original.capacity()) ||
		    !birth_passive_add(copy_live, copy_frames) || !reserve(copy_live, context))
			return false;
		std::copy(original.begin(), original.end(), candidate.begin() + 8);
		constexpr size_t role_offset =
			8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
		candidate[role_offset] = static_cast<uint8_t>(value.role);
		for (size_t i = 0; i < 4; ++i)
			candidate[role_offset + 1 + i] =
				static_cast<uint8_t>(value.configured_shop_matches >> (8 * i));
		*output = candidate;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	catch (...)
	{
		return false;
	}
}
bool birth_ordinary_flat_cash_role_current_bounded(const original_birth &birth,
						   bool (*reserve)(size_t, void *) noexcept,
						   void *context, size_t outer) noexcept
{
	if (!birth.cash_role_present ||
	    birth.cash_role.role != native_mobile_birth_cash_role::ordinary_wallet)
		return false;
	size_t base = outer;
	constexpr size_t capture_frames =
		sizeof(const quest_mobile_native_constructor_recipe *) +
		sizeof(native_mobile_birth_cash_role_recipe *) + sizeof(int) + sizeof(int) +
		sizeof(uint32_t) + sizeof(int) + sizeof(int) +
		sizeof(shop_native_mobile_birth_reset_selection) +
		sizeof(native_mobile_birth_cash_role_recipe) + sizeof(bool) +
		// Actual nested original shop selector: args/output, mobile/room/selected/shop/candidate.
		sizeof(int32_t) + sizeof(int32_t) + sizeof(int) +
		sizeof(shop_native_mobile_birth_reset_selection *) + sizeof(int) + sizeof(int) +
		sizeof(int) + sizeof(int) + sizeof(shop_native_mobile_birth_reset_selection) +
		sizeof(bool) +
		// Actual original binary RNUM lookup: virt/bot/top/mid/return.
		sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int);
	constexpr size_t frames =
		sizeof(&birth) + sizeof(reserve) + sizeof(context) + sizeof(outer) + sizeof(base) +
		sizeof(bool) + sizeof(native_mobile_birth_cash_role_recipe) +
		sizeof(native_mobile_birth_cash_role_recipe_bytes) +
		sizeof(native_mobile_birth_cash_role_recipe_bytes) + capture_frames;
	// Actual array== -> equal -> __equal_aux -> __equal_aux1 ->
	// __equal<true>::equal/__memcmp source carriers on installed GCC13.
	constexpr size_t equality_frames =
		sizeof(const native_mobile_birth_cash_role_recipe_bytes *) +
		sizeof(const native_mobile_birth_cash_role_recipe_bytes *) + sizeof(bool) +
		sizeof(const native_mobile_birth_cash_role_recipe_bytes *) +
		sizeof(const uint8_t *) + sizeof(const uint8_t *) + sizeof(const uint8_t *) +
		sizeof(bool) + sizeof(const uint8_t *) + sizeof(const uint8_t *) +
		sizeof(const uint8_t *) + sizeof(bool) + sizeof(const uint8_t *) +
		sizeof(const uint8_t *) + sizeof(const uint8_t *) + sizeof(bool) +
		sizeof(const uint8_t *) + sizeof(const uint8_t *) + sizeof(const uint8_t *) +
		sizeof(size_t) + sizeof(bool) + sizeof(const uint8_t *) + sizeof(const uint8_t *) +
		sizeof(size_t) + sizeof(int);
	if (!birth_passive_add(base, equality_frames))
		return false;
	if (!birth_passive_add(base, frames) || !reserve || !reserve(base, context))
		return false;
	native_mobile_birth_cash_role_recipe observed;
	native_mobile_birth_cash_role_recipe_bytes expected{}, actual{};
	return native_mobile_birth_cash_role_recipe_capture(birth.constructor, &observed) &&
	       birth_ordinary_flat_role_encode_bounded(birth.cash_role, &expected, reserve, context,
						       base) &&
	       birth_ordinary_flat_role_encode_bounded(observed, &actual, reserve, context, base) &&
	       expected == actual;
}

}

bool quest_mobile_native_birth_owner::ordinary_flat_freeze_source_current(size_t index) noexcept
{
	if (!nevent_is_game_thread() || index >= births.size() || !births[index])
		return false;
	const auto &b = *births[index];
	if (!b.npc_flat_factory_scope || !b.ordinary_flat_source ||
	    !b.ordinary_flat_source->state_ || !b.constructor_present || !b.sealed || b.cold ||
	    b.blocked || b.submitted || b.completed || b.retired || b.mobile_started ||
	    b.mobile_consumed || b.runtime_applied || b.physically_proven || !b.cash_role_present ||
	    b.cash_role.role != native_mobile_birth_cash_role::ordinary_wallet ||
	    !b.npc_flat_factory_scope->current() || !b.bindings.valid_flat())
		return false;
	const auto &pin = *b.ordinary_flat_source->state_;
	const auto &scope = *b.npc_flat_factory_scope;
	const auto &source = b.reference.birth_source;
	const auto &image = b.image.reference;
	// Both capabilities were privately installed by this very original M owner.
	// An equal operation/native ID or a caller-built value cannot mint either.
	return pin.producer == &b && pin.selected_root == scope.root_ &&
	       pin.runtime == scope.runtime_id_ && pin.instance == scope.native_id_ &&
	       pin.birth.bytes == scope.operation_.bytes && pin.slot == scope.source_.slot &&
	       pin.zone == scope.zone_ && pin.room == scope.room_ && pin.rnum == scope.rnum_ &&
	       pin.original_m_args[0] == scope.original_m_args_[0] &&
	       pin.original_m_args[1] == scope.original_m_args_[1] &&
	       pin.original_m_args[2] == scope.original_m_args_[2] &&
	       pin.original_m_args[3] == scope.original_m_args_[3] &&
	       pin.lineage.bytes == scope.lineage_.bytes && pin.epoch.bytes == scope.epoch_.bytes &&
	       source.kind == economic_source_kind::npc_generation && !source.sequence &&
	       source.source.bytes == pin.invocation.bytes &&
	       source.generation.bytes == pin.invocation.bytes &&
	       pin.birth.bytes == b.reference.birth_operation.bytes &&
	       pin.instance == b.reference.mobile_instance_id && pin.runtime == b.runtime_id &&
	       pin.slot == source.slot && pin.zone == b.zone && pin.room == b.room &&
	       pin.rnum == b.rnum && image.mobile_instance_id == b.reference.mobile_instance_id &&
	       image.birth_operation.bytes == b.reference.birth_operation.bytes &&
	       image.birth_source.kind == source.kind &&
	       image.birth_source.source.bytes == source.source.bytes &&
	       image.birth_source.generation.bytes == source.generation.bytes &&
	       image.birth_source.sequence == source.sequence &&
	       image.birth_source.slot == source.slot &&
	       image.mobile_vnum == b.reference.mobile_vnum &&
	       image.birthplace_vnum == b.reference.birthplace_vnum &&
	       image.reset_zone_vnum == b.reference.reset_zone_vnum &&
	       image.provenance == b.reference.provenance &&
	       image.mobile_revision == b.reference.mobile_revision &&
	       image.stock_revision == b.reference.stock_revision;
}

bool quest_mobile_native_birth_owner::freeze_ordinary_flat_command(size_t index, size_t outer_live)
{
	if (index >= births.size() || !births[index] || !births[index]->canonical.empty())
		return false;
	auto &b = *births[index];
	// Preserve the original role guard's scalar short-circuit and blocking law.
	if (!b.constructor_present || !b.cash_role_present ||
	    b.cash_role.role != native_mobile_birth_cash_role::ordinary_wallet)
	{
		b.blocked = true;
		return false;
	}
	struct freeze_budget
	{
		const critical_command *command;
		const std::vector<uint8_t> *canonical;
		size_t fixed;
		size_t index;
		const original_birth *owner;
	};
	const auto reserve = [](size_t extra, void *opaque) noexcept
	{
		const auto &state = *static_cast<const freeze_budget *>(opaque);
		if (state.index >= births.size() || births[state.index].get() != state.owner ||
		    !ordinary_flat_freeze_source_current(state.index))
			return false;
		size_t live = state.fixed;
		// Actual command capacities and canonical capacity coexist at every
		// prospective cut; no encoded-size estimate or remembered peak.
		return birth_passive_command_heap(live, *state.command) &&
		       birth_passive_add(live, state.canonical->capacity()) &&
		       birth_passive_add(live, extra) && birth_passive_add(live, sizeof(extra)) &&
		       birth_passive_add(live, sizeof(opaque)) &&
		       birth_passive_add(live, sizeof(&state)) &&
		       birth_passive_add(live, sizeof(live)) &&
		       birth_passive_add(live, sizeof(bool)) &&
		       // Actual heap census/checked-add scalar helper source carriers.
		       birth_passive_add(live, sizeof(size_t *) + sizeof(const critical_command *) +
						       sizeof(bool) + sizeof(size_t *) +
						       sizeof(size_t) + sizeof(size_t) +
						       sizeof(bool) + sizeof(size_t *) +
						       sizeof(size_t) + sizeof(bool)) &&
		       charge(live);
	};

	// The actual local candidate/carriers live together through build+encode.
	// Registry/native stage/input heaps remain in fresh charge(), not here.
	constexpr size_t source_frames =
		sizeof(size_t) + sizeof(const original_birth *) +
		sizeof(const quest_mobile_native_birth_ordinary_source_pin::implementation *) +
		sizeof(const quest_mobile_native_npc_flat_factory_scope *) +
		sizeof(const economic_source_event *) +
		sizeof(const quest_mobile_native_reference *) + sizeof(bool) +
		sizeof(const shop_trade_original_procedure_binding_stage *) + sizeof(const void *) +
		sizeof(const void *) + sizeof(int) +
		sizeof(bool) + // valid_flat binding/entry/number/return
		sizeof(const proclib_recovery_chain_stage *) + sizeof(const void *) + sizeof(int) +
		sizeof(const void *) + sizeof(void *) + sizeof(bool) + // chain current
		// Exact normal_iterator carriers for the two real binding vectors.
		sizeof(decltype(b.bindings.flat_scopes_.cbegin())) +
		sizeof(decltype(b.bindings.flat_scopes_.cend())) + sizeof(const void *) +
		sizeof(decltype(b.bindings.bindings_.cbegin())) +
		sizeof(decltype(b.bindings.bindings_.cend())) + sizeof(const void *) +
		// Private chain requests use the same one-pointer normal_iterator ABI;
		// previous-function pointer, proclib_chain_prev argument and scalar loop.
		sizeof(const void *) + sizeof(const void *) + sizeof(void *) + sizeof(int) +
		sizeof(int);
	size_t fixed = outer_live;
	if (!birth_passive_add(fixed, sizeof(index)) ||
	    !birth_passive_add(fixed, sizeof(outer_live)) ||
	    !birth_passive_add(fixed, sizeof(&b)) || !birth_passive_add(fixed, sizeof(fixed)) ||
	    !birth_passive_add(fixed, sizeof(critical_command)) ||
	    !birth_passive_add(fixed, sizeof(std::vector<uint8_t>)) ||
	    !birth_passive_add(fixed, sizeof(freeze_budget)) ||
	    !birth_passive_add(fixed, sizeof(std::span<const native_mobile_birth_item_recipe>)) ||
	    !birth_passive_add(fixed, sizeof(bool)) || !birth_passive_add(fixed, sizeof(reserve)) ||
	    !birth_passive_add(fixed, source_frames) ||
	    !birth_passive_add(fixed, npc_flat_factory_scope_current_frames()) || !charge(fixed))
		return false;
	{
		critical_command original;
		std::vector<uint8_t> canonical;
		freeze_budget budget{ &original, &canonical, fixed, index, &b };
		const std::span<const native_mobile_birth_item_recipe> recipes(b.recipes);
		if (!reserve(0, &budget))
			return false;
		if (!b.constructor_present ||
		    !birth_ordinary_flat_cash_role_current_bounded(b, reserve, &budget, 0))
		{
			b.blocked = true;
			return false;
		}
		if (economic_gameplay_authority::
			    prepare_native_mobile_birth_ordinary_wallet_flat_bounded(
				    b.image, recipes, b.cash_role, critical_source_site::zone_event,
				    b.accepted_usec, &original, b.npc_flat_factory_scope->lineage_,
				    b.npc_flat_factory_scope->epoch_, reserve, &budget, 0) !=
		    economic_accounting_error::ok)
			return false;
		if (!reserve(0, &budget) ||
		    critical_command_encode_bounded(original, &canonical, reserve, &budget, 0) !=
			    critical_command_codec_result::ok)
			return false;
		// Both destinations were included as real CURRENT registry storage at
		// every callback. After the two noexcept moves their heaps transfer once.
		b.command = std::move(original);
		b.canonical = std::move(canonical);
		if (!charge(fixed))
		{
			b.blocked = true;
			discard(index);
			return false;
		}
		return true;
	}
}

// The actual private bridge owns only its live local carriers. Original input,
// old destination, canonical command and registry/native heaps stay in charge().
struct quest_mobile_native_birth_owner::ordinary_flat_envelope_budget
{
	size_t index, fixed;
	const original_birth *owner;
	const native_mobile_birth_recovery_context *progress;
	const critical_native_recovery_envelope *candidate;
	const native_mobile_birth_recovery_item *row;
};

size_t quest_mobile_native_birth_owner::ordinary_flat_envelope_source_frames() noexcept
{
	// Actual pure source capsule observer, binding vectors, chain lookup, and
	// native scope CURRENT profile. These calls allocate no hidden containers.
	constexpr size_t source =
		sizeof(size_t) + sizeof(const original_birth *) +
		sizeof(const quest_mobile_native_birth_ordinary_source_pin::implementation *) +
		sizeof(const quest_mobile_native_npc_flat_factory_scope *) +
		sizeof(const economic_source_event *) +
		sizeof(const quest_mobile_native_reference *) + sizeof(bool) +
		sizeof(const shop_trade_original_procedure_binding_stage *) + sizeof(const void *) +
		sizeof(const void *) + sizeof(int) + sizeof(bool) +
		sizeof(const proclib_recovery_chain_stage *) + sizeof(const void *) + sizeof(int) +
		sizeof(const void *) + sizeof(void *) + sizeof(bool) +
		sizeof(decltype(std::declval<const shop_trade_original_procedure_binding_stage &>()
					.flat_scopes_.cbegin())) +
		sizeof(decltype(std::declval<const shop_trade_original_procedure_binding_stage &>()
					.flat_scopes_.cend())) +
		sizeof(const void *) +
		sizeof(decltype(std::declval<const shop_trade_original_procedure_binding_stage &>()
					.bindings_.cbegin())) +
		sizeof(decltype(std::declval<const shop_trade_original_procedure_binding_stage &>()
					.bindings_.cend())) +
		sizeof(const void *) + sizeof(const void *) + sizeof(const void *) +
		sizeof(void *) + sizeof(int) + sizeof(int) + sizeof(size_t);
	return source + npc_flat_factory_scope_current_frames();
}

bool quest_mobile_native_birth_owner::reserve_ordinary_flat_envelope(size_t extra,
								     void *opaque) noexcept
{
	const auto &state = *static_cast<const ordinary_flat_envelope_budget *>(opaque);
	if (state.index >= births.size() || births[state.index].get() != state.owner ||
	    !ordinary_flat_freeze_source_current(state.index))
		return false;
	size_t live = state.fixed;
	if ((state.progress && !birth_passive_context_heap(live, *state.progress)) ||
	    (state.candidate && !birth_passive_envelope_heap(live, *state.candidate)) ||
	    (state.row && !birth_passive_rows(live, state.row->effects.capacity(),
					      sizeof(native_mobile_birth_recovery_effect))))
		return false;
	constexpr size_t callback_frames =
		sizeof(extra) + sizeof(opaque) + sizeof(&state) + sizeof(live) + sizeof(bool) +
		// Actual context/envelope/command/capacity census and checked-add scalars.
		sizeof(size_t *) + sizeof(const native_mobile_birth_recovery_context *) +
		sizeof(const native_mobile_birth_recovery_item *) + sizeof(const void *) +
		sizeof(const void *) + sizeof(bool) + sizeof(size_t *) +
		sizeof(const critical_native_recovery_envelope *) + sizeof(bool) +
		sizeof(size_t *) + sizeof(const critical_command *) + sizeof(bool) +
		sizeof(size_t *) + sizeof(size_t) + sizeof(size_t) + sizeof(bool) +
		sizeof(size_t *) + sizeof(size_t) + sizeof(bool);
	return birth_passive_add(live, callback_frames) && birth_passive_add(live, extra) &&
	       charge(live);
}

namespace
{
// Exact source-carrier profiles for the original fresh default-allocator vectors.
// Real prospective heap requests are calculated separately from count and width.
template <typename T> constexpr size_t birth_envelope_vector_reserve_frames() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	constexpr size_t queries = sizeof(const V *) + sizeof(size_t) + sizeof(const V *) +
				   sizeof(size_t) + sizeof(const V *) + sizeof(size_t) +
				   sizeof(const A *) + sizeof(size_t) + sizeof(size_t) +
				   sizeof(size_t);
	constexpr size_t reserve = sizeof(V *) + sizeof(size_t) + sizeof(size_t) + sizeof(T *);
	constexpr size_t allocate = sizeof(V *) + sizeof(size_t) + sizeof(T *) + sizeof(A *) +
				    sizeof(size_t) + sizeof(T *) + sizeof(A *) + sizeof(size_t) +
				    sizeof(T *) + sizeof(A *) + sizeof(size_t) +
				    sizeof(const void *) + sizeof(T *) + sizeof(size_t) +
				    sizeof(void *);
	constexpr size_t relocate =
		sizeof(T *) + sizeof(T *) + sizeof(T *) + sizeof(A *) + sizeof(T *) + sizeof(T *) +
		sizeof(T *) + sizeof(T *) + sizeof(A *) + sizeof(T *) + sizeof(T *) + sizeof(T *) +
		sizeof(T *) + sizeof(A *) + sizeof(T *) + sizeof(T *) + sizeof(bool);
	constexpr size_t deallocate = sizeof(V *) + sizeof(T *) + sizeof(size_t) + sizeof(A *) +
				      sizeof(T *) + sizeof(size_t) + sizeof(A *) + sizeof(T *) +
				      sizeof(size_t) + sizeof(void *) + sizeof(size_t);
	return queries + reserve + allocate + relocate + deallocate;
}
template <typename T> constexpr size_t birth_envelope_vector_default_frames() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	// resize -> _M_default_append actual fixed scalars and allocator/relocate;
	// default initialization is __uninitialized_default_n_a -> default_n_1.
	constexpr size_t append = sizeof(V *) + sizeof(size_t) + sizeof(V *) + sizeof(size_t) +
				  sizeof(size_t) + sizeof(size_t) + sizeof(T *) + sizeof(T *) +
				  sizeof(size_t) + sizeof(T *) + sizeof(T *);
	constexpr size_t length = sizeof(const V *) + sizeof(size_t) + sizeof(const char *) +
				  sizeof(size_t) + sizeof(size_t) + sizeof(size_t) +
				  sizeof(size_t) + sizeof(const size_t *) + sizeof(const size_t *);
	constexpr size_t initialize = sizeof(T *) + sizeof(size_t) + sizeof(A *) + sizeof(T *) +
				      sizeof(T *) + sizeof(size_t) + sizeof(T *) + sizeof(T *) +
				      sizeof(size_t) + sizeof(T *) + sizeof(T *) + sizeof(T *) +
				      sizeof(void *) + sizeof(void *) + sizeof(bool);
	return append + length + initialize + birth_envelope_vector_reserve_frames<T>();
}
template <typename T> constexpr size_t birth_envelope_vector_move_frames() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	// Fitting rvalue push: real argument/forward/allocator construct, original
	// row's actual vector move constructor/reset and allocator references;
	// no fictional full-vector or row temporary is introduced here.
	return sizeof(V *) + sizeof(T *) + sizeof(V *) + sizeof(T *) + sizeof(A *) + sizeof(T *) +
	       sizeof(T *) + sizeof(T *) + sizeof(T *) + sizeof(T *) + sizeof(T *) + sizeof(T *) +
	       sizeof(T *) + sizeof(std::vector<native_mobile_birth_recovery_effect> *) +
	       sizeof(std::vector<native_mobile_birth_recovery_effect> *) +
	       sizeof(std::allocator<native_mobile_birth_recovery_effect> *) +
	       sizeof(std::allocator<native_mobile_birth_recovery_effect> *) + sizeof(void *) +
	       sizeof(void *) + sizeof(void *);
}
}

bool quest_mobile_native_birth_owner::prepare_ordinary_flat_envelope(size_t index,
								     size_t outer_live)
{
	if (index >= births.size() || !births[index] || !births[index]->envelope.attachment.empty())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)outer_live;
	return false;
#else
	auto &b = *births[index];
	size_t fixed = outer_live;
	constexpr size_t frames = sizeof(index) + sizeof(outer_live) + sizeof(&b) + sizeof(fixed) +
				  sizeof(native_mobile_birth_recovery_context) +
				  sizeof(critical_native_recovery_envelope) +
				  sizeof(ordinary_flat_envelope_budget) + sizeof(bool) +
				  sizeof(size_t);
	if (!birth_passive_add(fixed, frames) ||
	    !birth_passive_add(fixed, ordinary_flat_envelope_source_frames()) || !charge(fixed))
		return false;
	native_mobile_birth_recovery_context context;
	critical_native_recovery_envelope envelope;
	ordinary_flat_envelope_budget budget{ index, fixed, &b, &context, &envelope, nullptr };
	size_t request = 0;
	if (!birth_passive_rows(request, b.stock.size(),
				sizeof(native_mobile_birth_recovery_item)) ||
	    !birth_passive_add(
		    request,
		    birth_envelope_vector_reserve_frames<native_mobile_birth_recovery_item>()) ||
	    !reserve_ordinary_flat_envelope(request, &budget))
		return false;
	context.items.reserve(b.stock.size());
	{
		size_t loop_live = fixed;
		constexpr size_t loop_frames =
			sizeof(loop_live) + sizeof(decltype(b.stock.cbegin())) +
			sizeof(decltype(b.stock.cend())) + sizeof(const original_item *) +
			sizeof(native_mobile_birth_recovery_item);
		if (!birth_passive_add(loop_live, loop_frames))
			return false;
		budget.fixed = loop_live;
		for (const auto &item : b.stock)
		{
			native_mobile_birth_recovery_item row;
			budget.row = &row;
			row.object_uid = item.uid;
			request = 0;
			if (!birth_passive_rows(request, item.effects.size(),
						sizeof(native_mobile_birth_recovery_effect)) ||
			    !birth_passive_add(request,
					       birth_envelope_vector_default_frames<
						       native_mobile_birth_recovery_effect>()) ||
			    !reserve_ordinary_flat_envelope(request, &budget))
				return false;
			row.effects.resize(item.effects.size());
			if (!reserve_ordinary_flat_envelope(
				    birth_envelope_vector_move_frames<
					    native_mobile_birth_recovery_item>(),
				    &budget))
				return false;
			context.items.push_back(std::move(row));
			budget.row = nullptr;
		}
	}
	budget.fixed = fixed; // Actual row/iterator/loop carriers died; no cached peak.
	if (!birth_passive_add(fixed, sizeof(critical_command)))
		return false;
	budget.fixed = fixed; // Genuine deep-copy carrier now lives through return.
	request = 0;
	if (!critical_command_fresh_copy_request_bytes(b.command, &request) ||
	    !birth_passive_add(request, critical_command_copy_frame_bytes()) ||
	    !reserve_ordinary_flat_envelope(request, &budget))
		return false;
	// The real copy constructor owns the complete original four vector copy
	// allocations; its inline carrier is already admitted in fixed.
	auto command_copy = b.command;
	envelope.command = std::move(command_copy);
	envelope.revision = 1;
	envelope.phase = critical_native_recovery_phase::execution_pending;
	if (native_mobile_birth_cash_role_recovery_encode_bounded(
		    b.command, context, &envelope.attachment, &reserve_ordinary_flat_envelope,
		    &budget, 0) != economic_accounting_error::ok ||
	    !native_mobile_birth_cash_role_recovery_initial_bounded(
		    envelope, &reserve_ordinary_flat_envelope, &budget, 0))
		return false;
	// Admit actual transfer and the original rollback's default temporary
	// carriers BEFORE either destination changes. Rollback performs no admission.
	constexpr size_t transition_frames =
		sizeof(critical_native_recovery_envelope *) +
		sizeof(critical_native_recovery_envelope *) +
		sizeof(native_mobile_birth_recovery_context *) +
		sizeof(native_mobile_birth_recovery_context *) + sizeof(std::vector<uint8_t>) +
		sizeof(std::allocator<uint8_t>) +
		sizeof(std::vector<native_mobile_birth_recovery_item>) +
		sizeof(std::allocator<native_mobile_birth_recovery_item>) + sizeof(void *) +
		sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(void *) +
		sizeof(critical_native_recovery_envelope) +
		sizeof(native_mobile_birth_recovery_context);
	request = transition_frames;
	if (!birth_passive_add(request, critical_command_copy_frame_bytes()) ||
	    !reserve_ordinary_flat_envelope(request, &budget))
		return false;
	static_assert(std::is_nothrow_move_assignable_v<critical_native_recovery_envelope>);
	static_assert(std::is_nothrow_move_assignable_v<native_mobile_birth_recovery_context>);
	b.envelope = std::move(envelope);
	b.recovery = std::move(context);
	if (!charge(fixed))
	{
		b.envelope = {};
		b.recovery = {};
		return false;
	}
	return true;
#endif
}

bool quest_mobile_native_birth_owner::ordinary_flat_submission_role_current(
	size_t index, size_t outer_live) noexcept
{
	if (index >= births.size() || !births[index])
		return false;
	const auto &b = *births[index];
	size_t fixed = outer_live;
	constexpr size_t frames = sizeof(index) + sizeof(outer_live) + sizeof(&b) + sizeof(fixed) +
				  sizeof(ordinary_flat_envelope_budget) + sizeof(bool);
	if (!birth_passive_add(fixed, frames) ||
	    !birth_passive_add(fixed, ordinary_flat_envelope_source_frames()) || !charge(fixed))
		return false;
	ordinary_flat_envelope_budget budget{ index, fixed, &b, nullptr, nullptr, nullptr };
	return reserve_ordinary_flat_envelope(0, &budget) &&
	       birth_ordinary_flat_cash_role_current_bounded(b, &reserve_ordinary_flat_envelope,
							     &budget, 0);
}

bool quest_mobile_native_birth_owner::reserve_ordinary_flat_submission(size_t full,
								       void *opaque) noexcept
{
	const auto &state = *static_cast<const ordinary_flat_envelope_budget *>(opaque);
	if (state.index >= births.size() || births[state.index].get() != state.owner ||
	    !ordinary_flat_freeze_source_current(state.index))
		return false;
	// The authentic future ROOT dispatcher supplies the complete exclusive
	// prefix. Caller/ROOT/current-storage terms must not be added here again;
	// this leaf owns only its actual callback and pure source observer frames.
	size_t live = full;
	constexpr size_t frames = sizeof(full) + sizeof(opaque) + sizeof(&state) + sizeof(live) +
				  sizeof(bool) + sizeof(size_t *) + sizeof(size_t) + sizeof(bool);
	return birth_passive_add(live, frames) &&
	       birth_passive_add(live, ordinary_flat_envelope_source_frames()) && charge(live);
}

critical_submit_result
quest_mobile_native_birth_owner::submit_ordinary_flat_envelope(size_t index, size_t outer_live,
							       bool *current_after_submit) noexcept
{
	if (!current_after_submit)
		return critical_submit_result::invalid;
	*current_after_submit = false;
	if (index >= births.size() || !births[index])
		return critical_submit_result::invalid;
	auto &b = *births[index];
	size_t surviving = outer_live;
	critical_submit_result result = critical_submit_result::overloaded;
	constexpr size_t surviving_frames = sizeof(index) + sizeof(outer_live) +
					    sizeof(current_after_submit) + sizeof(&b) +
					    sizeof(surviving) + sizeof(result) + sizeof(bool);
	if (!birth_passive_add(surviving, surviving_frames))
		return result;
	{
		using submission_guard = zone_reset_item_owner::ordinary_flat_submission_guard;
		size_t scope_live = surviving;
		size_t current_coordinator_bytes = SIZE_MAX;
		constexpr size_t scope_frames = sizeof(scope_live) +
						sizeof(current_coordinator_bytes) +
						sizeof(ordinary_flat_envelope_budget);
		// Actual guard storage is admitted once BEFORE construction. Registry,
		// canonical/envelope, journal and genuine outside C/G belong to charge,
		// not to this provider outer. No unavailable-current value becomes zero.
		if (!birth_passive_add(scope_live, scope_frames) ||
		    !birth_passive_add(scope_live, submission_guard::inline_bytes()))
			return result;
		constexpr size_t accessor_frames = sizeof(bool (*)(size_t, void *) noexcept) +
						   sizeof(submission_guard *) + sizeof(void *);
		// Actual accessor expression and scalar guard cleanup are prospectively
		// admitted before guard.begin, not first charged after call evaluation.
		constexpr size_t cleanup_frames = sizeof(submission_guard *) + sizeof(void *) +
						  sizeof(bool) + sizeof(critical_submit_result) +
						  sizeof(bool);
		size_t call_live = scope_live;
		if (!birth_passive_add(call_live, sizeof(call_live)) ||
		    !birth_passive_add(call_live, accessor_frames) ||
		    !birth_passive_add(call_live, cleanup_frames))
			return result;
		{
			size_t initial = call_live;
			constexpr size_t lifecycle_frames =
				sizeof(submission_guard *) +
				sizeof(bool (*)(size_t, void *) noexcept) + sizeof(void *) +
				sizeof(submission_guard *) + sizeof(void *) + sizeof(bool) +
				sizeof(size_t) + sizeof(size_t *) + sizeof(size_t) + sizeof(bool);
			if (!birth_passive_add(initial, sizeof(initial)) ||
			    !birth_passive_add(initial, lifecycle_frames) ||
			    !birth_passive_add(initial, ordinary_flat_envelope_source_frames()) ||
			    !charge(initial) || !ordinary_flat_freeze_source_current(index))
				return result;
		}
		ordinary_flat_envelope_budget budget{ index,   scope_live, &b,
						      nullptr, nullptr,	   nullptr };
		{
			submission_guard guard(&reserve_ordinary_flat_submission, &budget);
			if (guard.begin(call_live))
				result = critical_native_mobile_birth_publication_owner::
					submit_bounded(b.envelope, guard.callback(),
						       guard.context(), call_live,
						       &current_coordinator_bytes);
			// Preserve the actual returned disposition BEFORE any fallible
			// outside census. These are the original scalar result transitions.
			if (critical_submit_result_keeps_operation(result))
				b.submitted = true;
			else if (result == critical_submit_result::invalid ||
				 result == critical_submit_result::identity_conflict)
				b.blocked = true;
		} // Real submit mutex/lender already released; guard scalar scope ends.
	} // Callback context/output/query scalars died; no stale G/scratch retained.
	// No callback between real scope death and fresh outside observation.
	// Failure preserves actual result/state and asks pulse to stop this attempt
	// before any generation/completion/publication work.
	*current_after_submit = charge(surviving);
	return result;
}

// Retained ordinary source observation only, after genuine submission.
// The exact original privately installed source/scope/image/binding proofs
// remain mandatory. Completed births can still need generation observation;
// pre-submit freeze/submit gates and world/publication permissions are unchanged.
bool quest_mobile_native_birth_owner::ordinary_flat_observation_source_current(size_t index) noexcept
{
	if (!nevent_is_game_thread() || index >= births.size() || !births[index])
		return false;
	const auto &b = *births[index];
	if (!b.npc_flat_factory_scope || !b.ordinary_flat_source ||
	    !b.ordinary_flat_source->state_ || !b.constructor_present || !b.sealed || b.cold ||
	    b.blocked || !b.submitted || b.retired || b.mobile_started || b.mobile_consumed ||
	    b.runtime_applied || b.physically_proven || !b.cash_role_present ||
	    b.cash_role.role != native_mobile_birth_cash_role::ordinary_wallet ||
	    !b.npc_flat_factory_scope->current() || !b.bindings.valid_flat())
		return false;
	const auto &pin = *b.ordinary_flat_source->state_;
	const auto &scope = *b.npc_flat_factory_scope;
	const auto &source = b.reference.birth_source;
	const auto &image = b.image.reference;
	// Both capabilities were privately installed by this very original M owner.
	// An equal operation/native ID or a caller-built value cannot mint either.
	return pin.producer == &b && pin.selected_root == scope.root_ &&
	       pin.runtime == scope.runtime_id_ && pin.instance == scope.native_id_ &&
	       pin.birth.bytes == scope.operation_.bytes && pin.slot == scope.source_.slot &&
	       pin.zone == scope.zone_ && pin.room == scope.room_ && pin.rnum == scope.rnum_ &&
	       pin.original_m_args[0] == scope.original_m_args_[0] &&
	       pin.original_m_args[1] == scope.original_m_args_[1] &&
	       pin.original_m_args[2] == scope.original_m_args_[2] &&
	       pin.original_m_args[3] == scope.original_m_args_[3] &&
	       pin.lineage.bytes == scope.lineage_.bytes && pin.epoch.bytes == scope.epoch_.bytes &&
	       source.kind == economic_source_kind::npc_generation && !source.sequence &&
	       source.source.bytes == pin.invocation.bytes &&
	       source.generation.bytes == pin.invocation.bytes &&
	       pin.birth.bytes == b.reference.birth_operation.bytes &&
	       pin.instance == b.reference.mobile_instance_id && pin.runtime == b.runtime_id &&
	       pin.slot == source.slot && pin.zone == b.zone && pin.room == b.room &&
	       pin.rnum == b.rnum && image.mobile_instance_id == b.reference.mobile_instance_id &&
	       image.birth_operation.bytes == b.reference.birth_operation.bytes &&
	       image.birth_source.kind == source.kind &&
	       image.birth_source.source.bytes == source.source.bytes &&
	       image.birth_source.generation.bytes == source.generation.bytes &&
	       image.birth_source.sequence == source.sequence &&
	       image.birth_source.slot == source.slot &&
	       image.mobile_vnum == b.reference.mobile_vnum &&
	       image.birthplace_vnum == b.reference.birthplace_vnum &&
	       image.reset_zone_vnum == b.reference.reset_zone_vnum &&
	       image.provenance == b.reference.provenance &&
	       image.mobile_revision == b.reference.mobile_revision &&
	       image.stock_revision == b.reference.stock_revision;
}

struct quest_mobile_native_birth_owner::ordinary_flat_observation_budget
{
	ordinary_flat_envelope_budget prefix;
	bool denied = false;
};

bool quest_mobile_native_birth_owner::reserve_ordinary_flat_observation(size_t full,
									void *opaque) noexcept
{
	auto &budget = *static_cast<ordinary_flat_observation_budget *>(opaque);
	const auto &state = budget.prefix;
	size_t live = full;
	// Genuine retained-source leaf, with its complete actual callback carriers.
	// It never delegates to the pre-submit-only freeze/submit callback.
	constexpr size_t frames = sizeof(full) + sizeof(opaque) + sizeof(&budget) + sizeof(&state) +
				  sizeof(live) + sizeof(bool) + sizeof(size_t *) + sizeof(size_t) +
				  sizeof(bool);
	if (state.index >= births.size() || births[state.index].get() != state.owner ||
	    !ordinary_flat_observation_source_current(state.index) ||
	    !birth_passive_add(live, frames) ||
	    !birth_passive_add(live, ordinary_flat_envelope_source_frames()) || !charge(live))
	{
		budget.denied = true;
		return false;
	}
	return true;
}

bool quest_mobile_native_birth_owner::observe_ordinary_flat_post_submit(size_t index,
									size_t outer_live) noexcept
{
	if (index >= births.size() || !births[index])
		return false;
	auto &b = *births[index];
	if (!b.npc_flat_factory_scope || !b.submitted)
		return false;
	// Admit actual completion/generation outputs BEFORE constructing them.
	// They remain live through both observations and the immediate outside
	// census. Registry-held old outputs remain in the genuine current charge.
	struct observation_outputs
	{
		critical_completion completion{};
		uint64_t generation = 0;
		bool generation_available = false, completion_available = false;
		bool scope_ready = false, denied = false, invalid_receipt = false;
	};
	size_t surviving = outer_live;
	constexpr size_t surviving_frames = sizeof(index) + sizeof(outer_live) + sizeof(&b) +
					    sizeof(surviving) + sizeof(observation_outputs) +
					    sizeof(unsigned int) + sizeof(bool);
	if (!birth_passive_add(surviving, surviving_frames))
		return false;
	{
		size_t initial = surviving;
		// Genuine source observer and charge helper/output carriers before the
		// output constructor. Existing full native/library profiles stay OPEN.
		if (!birth_passive_add(initial, sizeof(initial)) ||
		    !birth_passive_add(initial, sizeof(size_t *) + sizeof(size_t) + sizeof(bool)) ||
		    !birth_passive_add(initial, ordinary_flat_envelope_source_frames()) ||
		    !charge(initial) || !ordinary_flat_observation_source_current(index))
			return false;
	}
	observation_outputs outputs;
	// Each real observation has its own actual guard lifetime. Even an early
	// coordinator refusal before the birth callback runs must be followed by a
	// checked outside CURRENT census before the next observation is attempted.
	for (unsigned int observation = 0; observation < 2; ++observation)
	{
		if (observation == 0 ? b.coordinator_generation != 0 : b.completed)
			continue;
		outputs.generation_available = false;
		outputs.completion_available = false;
		outputs.scope_ready = false;
		outputs.denied = false;
		{
			using observation_guard =
				zone_reset_item_owner::ordinary_flat_submission_guard;
			size_t scope_live = surviving;
			size_t current_coordinator_bytes = SIZE_MAX;
			constexpr size_t scope_frames = sizeof(scope_live) +
							sizeof(current_coordinator_bytes) +
							sizeof(ordinary_flat_observation_budget);
			if (!birth_passive_add(scope_live, scope_frames) ||
			    !birth_passive_add(scope_live, observation_guard::inline_bytes()))
				return false;
			// Actual guard accessors, destructor and genuine callback wrapper are
			// admitted before construction. No historical maximum stands in for C/G.
			constexpr size_t accessor_frames =
				sizeof(bool (*)(size_t, void *) noexcept) +
				sizeof(observation_guard *) + sizeof(void *);
			constexpr size_t cleanup_frames = sizeof(observation_guard *) +
							  sizeof(void *) + sizeof(bool) +
							  sizeof(bool);
			size_t call_live = scope_live;
			if (!birth_passive_add(call_live, sizeof(call_live)) ||
			    !birth_passive_add(call_live, accessor_frames) ||
			    !birth_passive_add(call_live, cleanup_frames))
				return false;
			{
				size_t initial = call_live;
				constexpr size_t lifecycle_frames =
					sizeof(observation_guard *) +
					sizeof(bool (*)(size_t, void *) noexcept) + sizeof(void *) +
					sizeof(observation_guard *) + sizeof(void *) +
					sizeof(bool) + sizeof(size_t) + sizeof(size_t *) +
					sizeof(size_t) + sizeof(bool);
				if (!birth_passive_add(initial, sizeof(initial)) ||
				    !birth_passive_add(initial, lifecycle_frames) ||
				    !birth_passive_add(initial,
						       ordinary_flat_envelope_source_frames()) ||
				    !charge(initial) ||
				    !ordinary_flat_observation_source_current(index))
					return false;
			}
			ordinary_flat_observation_budget budget{
				{ index, scope_live, &b, nullptr, nullptr, nullptr }, false
			};
			{
				observation_guard guard(&reserve_ordinary_flat_observation,
							&budget);
				outputs.scope_ready = guard.begin(call_live);
				if (outputs.scope_ready)
				{
					if (observation == 0)
						outputs.generation_available =
							critical_native_mobile_birth_publication_owner::
								observe_generation_bounded(
									b.envelope,
									&outputs.generation,
									guard.callback(),
									guard.context(), call_live,
									&current_coordinator_bytes);
					else
						outputs.completion_available =
							critical_native_mobile_birth_publication_owner::
								completion_bounded(
									b.command.operation_id,
									&outputs.completion,
									guard.callback(),
									guard.context(), call_live,
									&current_coordinator_bytes);
				}
				outputs.denied = budget.denied;
			} // Actual coor mutex/lender returned, then the genuine common guard ends.
		} // Callback/query/call-local carriers died; no stale G or scratch survives.
		// Preserve successfully observed original scalar transitions before the
		// fallible outside census. A missing receipt makes no state transition.
		if (outputs.generation_available)
			b.coordinator_generation = outputs.generation;
		if (outputs.completion_available)
		{
			if (!critical_completion_disposition_valid(outputs.completion))
			{
				b.blocked = true;
				outputs.invalid_receipt = true;
			}
			else
			{
				b.completion = outputs.completion;
				b.completed = true;
			}
		}
		// Exact post-scope CURRENT ownership is observed even after ordinary absence
		// or a denied callback. Never proceed using unavailable C/G as logical zero.
		if (!charge(surviving) || !outputs.scope_ready || outputs.denied ||
		    outputs.invalid_receipt)
			return false;
	} // Ordinary absence can advance only after this fresh outside census.
	return true;
}

namespace
{
// Actual published passive ownership observers, summed per complete source
// scope, including real desugared range/begin/end/row carriers. Heap payloads
// remain separate actual capacities in replay_workspace::current_bytes.
constexpr size_t ordinary_passive_observer_frames =
	// current_bytes(this/output/bytes), add(value/extra/result), rows(value/count/width/result).
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(size_t *) + sizeof(size_t) +
	sizeof(bool) + sizeof(size_t *) + 2 * sizeof(size_t) + sizeof(bool) +
	// command_heap(value/command/fresh/result), envelope_heap same original source.
	2 * (2 * sizeof(void *) + 2 * sizeof(bool)) +
	// context_heap(value/progress + range reference/begin/end/item + result).
	6 * sizeof(void *) + sizeof(bool) +
	// text_heap(value/text/result), image_heap nested real ranges and row refs.
	2 * sizeof(void *) + sizeof(bool) + 10 * sizeof(void *) + sizeof(bool) +
	// recipes_heap and stock_heap complete range/begin/end/row sources.
	2 * (6 * sizeof(void *) + sizeof(bool)) +
	// new_body_heap(value/body/result), push_request values/request/increment/capacity/bytes/max queries.
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	8 * (sizeof(void *) + sizeof(size_t));

template <class T> constexpr size_t ordinary_passive_vector_copy_frames() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	using I = typename V::const_iterator;
	return
		// vector(copy): this/source/base allocator plus size queries and _M_initialize_dispatch.
		2 * sizeof(V *) + sizeof(A) + 2 * (sizeof(V *) + sizeof(size_t)) +
		// _Vector_base/_Vector_impl/_Vector_impl_data and _M_create_storage.
		4 * sizeof(void *) + sizeof(A *) + sizeof(size_t) +
		// begin/end normal iterator constructor(base reference)/return, allocator getter.
		2 * (sizeof(V *) + sizeof(I) + sizeof(I *) + sizeof(T **)) + sizeof(A *) +
		// uninitialized_copy_a(first,last,result,allocator,current) -> uninitialized_copy
		// -> __uninitialized_copy<true>::__uninit_copy, std::copy and copy_move_a chain.
		2 * sizeof(I) + 2 * sizeof(T *) + sizeof(A *) + 2 * sizeof(I) + sizeof(T *) +
		2 * sizeof(I) + sizeof(T *) + 2 * sizeof(I) + sizeof(T *) +
		3 * (2 * sizeof(I) + sizeof(T *)) +
		// niter_base(first)/base(this)/miter_base(first); wrap(original,result).
		4 * (sizeof(I) + sizeof(I *) + sizeof(const T *)) + sizeof(I) + sizeof(const T *) +
		sizeof(I) +
		// bulk copy source first/last/result/count, memcpy arguments/return.
		2 * sizeof(const T *) + sizeof(T *) + sizeof(std::ptrdiff_t) + sizeof(void *) +
		sizeof(const void *) + sizeof(size_t) + sizeof(void *) +
		// _M_allocate and allocator/new_allocator source (including returned pointer).
		sizeof(V *) + sizeof(size_t) + sizeof(T *) + sizeof(A *) + sizeof(size_t) +
		sizeof(T *) + sizeof(A *) + sizeof(size_t) + sizeof(const void *) + sizeof(A *) +
		sizeof(size_t) + sizeof(size_t) + sizeof(void *) +
		// failure rollback Destroy(first,last,allocator), deallocate original actual bytes.
		2 * sizeof(T *) + sizeof(A *) + sizeof(V *) + sizeof(T *) + sizeof(size_t) +
		sizeof(A *) + sizeof(T *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t);
}

template <class T> constexpr size_t ordinary_passive_vector_move_frames() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	return
		// operator=(vector&&), _M_move_assign(true_type), its real empty tmp/base.
		2 * sizeof(V *) + sizeof(V *) + sizeof(V *) + sizeof(std::true_type) + sizeof(V) +
		sizeof(A *) + 4 * sizeof(void *) +
		// _M_swap_data(other,this,tmp), three actual _M_copy_data(this/source) calls.
		2 * sizeof(void *) + 3 * sizeof(T *) + 3 * (2 * sizeof(void *)) +
		// allocator_on_move actual two refs; moved-from destructor/deallocation scopes.
		2 * sizeof(A *) + sizeof(V *) + 2 * sizeof(T *) + sizeof(A *) + sizeof(V *) +
		sizeof(T *) + sizeof(size_t) + sizeof(A *) + sizeof(T *) + sizeof(size_t);
}

// Four genuine nested vectors are always empty in this passive owner:
// mobile.shared_affect_installed_, bindings.bindings_, bindings.flat_scopes_,
// and bindings.chain_.requests_. Their inline objects already belong to
// sizeof(original_birth); these are only actual constructor/cleanup carriers.
constexpr size_t ordinary_passive_nested_empty_vector_frames =
	4 * (
		    // vector(default), _Vector_base(default), _Vector_impl(), allocator(),
		    // __new_allocator(), and _Vector_impl_data(): six genuine this pointers.
		    6 * sizeof(void *) +
		    // ~vector(this); _M_get_Tp_allocator(this,result-reference).
		    sizeof(void *) + 2 * sizeof(void *) +
		    // _Destroy(first,last,allocator) -> _Destroy(first,last) ->
		    // _Destroy_aux<true/false>::__destroy(first,last). All ranges are empty,
		    // so even shared_ptr's nontrivial element destructor is never entered.
		    3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
		    // ~_Vector_base(this) -> _M_deallocate(this,p,n). The real null-p guard
		    // returns without allocator deallocation or any request for empty storage.
		    sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t)) +
	// Chain's genuine destructor(this) -> reset(this) -> free(allocation=null).
	2 * sizeof(proclib_recovery_chain_stage *) + sizeof(void *) +
	// requests_.clear(this) -> _M_erase_at_end(this,pos,n). Empty n==0
	// returns before its destroy tail; vector destruction remains charged above.
	3 * sizeof(void *) + sizeof(size_t);

constexpr size_t ordinary_passive_body_frames =
	// make_unique and exact original_birth value ctor/new result;
	// unique_ptr(pointer) ctor/__uniq_ptr_impl, actual tuple get pointer slots.
	sizeof(original_birth *) + sizeof(original_birth *) + sizeof(size_t) + 4 * sizeof(void *) +
	sizeof(original_birth *) +
	// original_birth's empty vectors/default bases/allocators (image, recipes,
	// command4, custody, canonical, envelope5, recovery1, stock = 15 vectors).
	15 * (5 * sizeof(void *)) +
	// empty string/default allocator hider/local data/length setter.
	6 * sizeof(void *) + sizeof(size_t) +
	// original fixed native stages/binding/shared_ptr default constructors.
	8 * sizeof(void *) + ordinary_passive_nested_empty_vector_frames +
	// unique_ptr move/default/delete ctor and impl reset/release/deleter accesses;
	// tuple/head-base getters expose real pointer/reference carriers (both indices).
	10 * sizeof(void *) + sizeof(original_birth *) + 8 * (2 * sizeof(void *) + sizeof(void *)) +
	// actual registry empty slot move or vector push/emplace/_M_realloc_insert,
	// unique_ptr vector element ctor/relocation and old destructors.
	12 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(std::ptrdiff_t) +
	birth_envelope_vector_reserve_frames<std::unique_ptr<original_birth>>();

constexpr size_t ordinary_passive_string_frames =
	// operator=/assign/_M_assign, source addressof and length/capacity accessors.
	6 * sizeof(std::string *) + sizeof(const std::string *) + 3 * sizeof(size_t) +
	sizeof(char *) + sizeof(const std::string *) + sizeof(const std::string *) +
	// _M_create(this,requested&,old_capacity) and allocator_traits/_S_allocate.
	sizeof(std::string *) + sizeof(size_t *) + sizeof(size_t) + sizeof(char *) +
	sizeof(std::allocator<char> *) + sizeof(size_t) + sizeof(const void *) +
	sizeof(std::allocator<char> *) + sizeof(size_t) + sizeof(void *) +
	// _M_dispose/_M_destroy/_M_data/_M_capacity and _S_copy/traits::copy/_M_set_length.
	4 * sizeof(std::string *) + sizeof(size_t) + sizeof(char *) + sizeof(size_t) +
	sizeof(char *) + sizeof(const char *) + sizeof(size_t) + sizeof(char *) +
	sizeof(const char *) + sizeof(size_t) + sizeof(std::string *) + sizeof(size_t) +
	sizeof(char *) + sizeof(size_t) + sizeof(void *) + sizeof(const void *) + sizeof(size_t) +
	sizeof(void *) +
	// Actual deallocate source; selected success/string move and unique_ptr move.
	sizeof(std::allocator<char> *) + sizeof(char *) + sizeof(size_t) + 8 * sizeof(void *) +
	3 * sizeof(size_t) + sizeof(bool);

}

bool quest_mobile_native_birth_owner::restore_ordinary_flat_bounded(
	const critical_native_recovery_envelope &envelope, const std::string &configured_root,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	// Passive registration only, called by the genuine coordinator startup owner
	// while its real init lock/lender and configured recovery root are retained.
	// It neither reenters coordinator state nor constructs a warm factory pin.
	const char *actual_root = persistence_mode_flatfile_root();
	if (!reserve || !nevent_is_game_thread() || persistence_mode_requires_mysql() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY || !actual_root ||
	    !*actual_root || configured_root != actual_root ||
	    !ordinary_wallet_command(envelope.command) || shared_shop_command(envelope.command))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	struct replay_workspace
	{
		native_mobile_birth_recovery_context progress;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		critical_native_recovery_envelope retained;
		std::vector<uint8_t> canonical;
		std::unique_ptr<original_birth> body;
		size_t base = 0, current = 0, request = 0, slot = 0, scan = 0, row = 0, old_row = 0;
		bool current_bytes(size_t *output) const noexcept
		{
			size_t bytes = base;
			if (!output || !birth_passive_context_heap(bytes, progress) ||
			    !birth_passive_image_heap(bytes, image) ||
			    !birth_passive_recipes_heap(bytes, recipes) ||
			    !birth_passive_envelope_heap(bytes, retained) ||
			    !birth_passive_add(bytes, canonical.capacity()) ||
			    (body && !birth_passive_new_body_heap(bytes, *body)))
				return false;
			*output = bytes;
			return true;
		}
	};
	const size_t frames =
		sizeof(replay_workspace) + sizeof(critical_command) +
		sizeof(critical_native_recovery_envelope) +
		// entry parameters/root/output/span/body and source-conflict refs, local scalar queries.
		18 * sizeof(void *) + 12 * sizeof(size_t) + 8 * sizeof(bool) + 4 * sizeof(int) +
		ordinary_passive_observer_frames + ordinary_passive_body_frames +
		ordinary_passive_string_frames + critical_command_copy_frame_bytes() +
		ordinary_passive_vector_copy_frames<uint8_t>() +
		ordinary_passive_vector_move_frames<player_item_snapshot>() +
		ordinary_passive_vector_move_frames<native_mobile_birth_item_recipe>() +
		3 * ordinary_passive_vector_move_frames<uint8_t>() +
		ordinary_passive_vector_move_frames<native_mobile_birth_recovery_item>() +
		birth_envelope_vector_default_frames<item_ownership_runtime_entry>() +
		birth_envelope_vector_default_frames<original_item>() +
		// original_item default ctor unique_ptr + effects vector; no native stage is created.
		9 * sizeof(void *);
	size_t base = outer;
	if (!birth_passive_add(base, frames) || !birth_passive_admit(base, 0, reserve, context))
		return false;
	replay_workspace work;
	work.base = base;
	try
	{
		const std::span<const uint8_t> attachment = envelope.attachment;
		if (native_mobile_birth_cash_role_recovery_validate_bounded(
			    envelope, reserve, context, base) != economic_accounting_error::ok ||
		    native_mobile_birth_cash_role_recovery_decode_status_bounded(
			    envelope.command, attachment, &work.progress, reserve, context, base) !=
			    economic_accounting_error::ok ||
		    !work.current_bytes(&work.current) ||
		    critical_command_encode_bounded(envelope.command, &work.canonical, reserve,
						    context, work.current) !=
			    critical_command_codec_result::ok)
			return false;
		for (work.scan = 0; work.scan < births.size(); ++work.scan)
			if (births[work.scan] &&
			    births[work.scan]->reference.birth_operation.bytes ==
				    envelope.command.operation_id.bytes)
			{
				const auto &previous = *births[work.scan];
				// Exact original duplicate policy: passive return does not retag an existing
				// warm entry, mint a source pin or install a new configured-root owner.
				return previous.envelope.revision == envelope.revision &&
				       previous.envelope.phase == envelope.phase &&
				       previous.envelope.attachment == envelope.attachment &&
				       previous.canonical == work.canonical;
			}
		if (!work.current_bytes(&work.current) ||
		    native_mobile_birth_cash_role_command_decode_bounded(
			    envelope.command, &work.image, &work.recipes, &work.role, reserve,
			    context, work.current) != economic_accounting_error::ok ||
		    work.role.role != native_mobile_birth_cash_role::ordinary_wallet)
			return false;
		work.request = 0;
		if (!birth_passive_envelope_heap(work.request, envelope, true) ||
		    !work.current_bytes(&work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		// Genuine full original copy, then no-throw move. This selects the reviewed
		// command copy constructor rather than an unaccounted vector-copy assignment.
		{
			auto envelope_copy = envelope;
			work.retained = std::move(envelope_copy);
		}
		// Original complete birth/source/UID conflict policy before registry transfer.
		for (work.scan = 0; work.scan < births.size(); ++work.scan)
			if (births[work.scan])
			{
				const auto &previous = *births[work.scan];
				const auto &left = previous.reference.birth_source;
				const auto &right = work.image.reference.birth_source;
				if (previous.reference.mobile_instance_id ==
					    work.image.reference.mobile_instance_id ||
				    (left.kind == right.kind &&
				     left.source.bytes == right.source.bytes &&
				     left.generation.bytes == right.generation.bytes &&
				     left.sequence == right.sequence && left.slot == right.slot))
					return false;
				for (work.row = 0; work.row < work.image.items.size(); ++work.row)
					for (work.old_row = 0;
					     work.old_row < previous.image.items.size();
					     ++work.old_row)
						if (work.image.items[work.row].object_uid ==
						    previous.image.items[work.old_row].object_uid)
							return false;
			}
		while (work.slot < births.size() && births[work.slot])
			++work.slot;
		if (work.slot == births.size() &&
		    births.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS)
			return false;
		if (!work.current_bytes(&work.current) ||
		    !birth_passive_admit(work.current, sizeof(original_birth), reserve, context))
			return false;
		work.body = std::make_unique<original_birth>();
		auto &body = *work.body;
		body.reference = work.image.reference;
		body.image = std::move(work.image);
		body.recipes = std::move(work.recipes);
		body.cash_role = work.role;
		body.cash_role_present = true;
		body.constructor = work.role.original;
		body.constructor_present = true;
		if (body.constructor.wire_version ==
			    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
		    body.constructor.wire_version ==
			    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
		{
			if (body.constructor.reset_room_vnum != body.reference.birthplace_vnum)
				return false;
			body.shop = body.constructor.reset_shop_index;
		}
		body.rnum = real_mobile(body.reference.mobile_vnum);
		body.zone = real_zone(body.reference.reset_zone_vnum);
		body.room = real_room0(body.reference.birthplace_vnum);
		if (body.rnum < 0 || body.zone < 0 || body.room < 0)
			return false;
		work.request = 0;
		if (!birth_passive_command_heap(work.request, envelope.command, true) ||
		    !work.current_bytes(&work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		{
			auto command_copy = envelope.command;
			body.command = std::move(command_copy);
		}
		body.canonical = std::move(work.canonical);
		body.envelope = std::move(work.retained);
		body.recovery = std::move(work.progress);
		// Original copy assignment into a fresh GCC13 local-capacity15 string:
		// _M_create doubles15 to30 for length16..29 before capacity+1.
		const size_t root_length = configured_root.size();
		const size_t root_capacity = root_length < 30 ? 30 : root_length;
		if (root_capacity == SIZE_MAX)
			return false;
		work.request = root_length <= 15 ? 0 : root_capacity + 1;
		if (configured_root.size() == SIZE_MAX || !work.current_bytes(&work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		body.ordinary_flat_recovery_root = configured_root;
		body.ordinary_flat_replay_registered = true;
		body.cold = true;
		body.cold_replay_enrolled = true;
		body.sealed = true;
		body.submitted = true;
		work.request = 0;
		if (!birth_passive_rows(work.request, body.image.items.size(),
					sizeof(item_ownership_runtime_entry)) ||
		    !birth_passive_rows(work.request, body.recovery.items.size(),
					sizeof(original_item)) ||
		    !work.current_bytes(&work.current) ||
		    !birth_passive_admit(work.current, work.request, reserve, context))
			return false;
		body.current_custody.resize(body.image.items.size());
		body.stock.resize(body.recovery.items.size());
		for (work.row = 0; work.row < body.recovery.items.size(); ++work.row)
		{
			work.scan = 0;
			while (work.scan < body.image.items.size() &&
			       body.image.items[work.scan].object_uid !=
				       body.recovery.items[work.row].object_uid)
				++work.scan;
			if (work.scan == body.image.items.size())
				return false;
			body.stock[work.row].uid = body.image.items[work.scan].object_uid;
			body.stock[work.row].rnum = real_object(body.image.items[work.scan].vnum);
			if (body.stock[work.row].rnum < 0)
				return false;
		}
		if (work.slot == births.size())
		{
			if (!birth_passive_push_request(births, &work.request) ||
			    !work.current_bytes(&work.current) ||
			    !birth_passive_admit(work.current, work.request, reserve, context))
				return false;
			births.push_back(std::move(work.body));
		}
		else
			births[work.slot] = std::move(work.body);
		static_assert(std::is_nothrow_move_assignable_v<critical_native_recovery_envelope>);
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		// New owning body transferred once. Observe actual registry storage through
		// the received startup lender before success; rollback only this new entry.
		if (!work.current_bytes(&work.current) ||
		    !birth_passive_admit(work.current, 0, reserve, context))
		{
			births[work.slot].reset();
			if (work.current_bytes(&work.current))
				(void)birth_passive_admit(work.current, 0, reserve, context);
			return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_mobile_native_birth_restore_ordinary_bounded(
	const critical_native_recovery_envelope &envelope, const std::string &configured_root,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	size_t live = outer_live;
	return birth_passive_add(live, sizeof(live) + 4 * sizeof(void *) + sizeof(outer_live)) &&
	       birth_passive_admit(live, 0, reserve, context) &&
	       quest_mobile_native_birth_owner::restore_ordinary_flat_bounded(
		       envelope, configured_root, reserve, context, live);
}

namespace
{
// Same original complete owning-value walk, with SIZE_MAX checked totals for a
// pure measurement. Admission/capacity policy stays with the actual ROOT lender.
bool birth_current_add(size_t &value, size_t part) noexcept
{
	if (part > SIZE_MAX - value)
	{
		errno = EOVERFLOW;
		return false;
	}
	value += part;
	return true;
}

bool birth_current_rows(size_t &value, size_t count, size_t width) noexcept
{
	if (width && count > SIZE_MAX / width)
	{
		errno = EOVERFLOW;
		return false;
	}
	return birth_current_add(value, count * width);
}

bool birth_current_command_heap(size_t &value, const critical_command &command,
				bool fresh_copy = false) noexcept
{
	return birth_current_rows(value, fresh_copy ? command.keys.size() : command.keys.capacity(),
				  sizeof(critical_entity_key)) &&
	       birth_current_rows(value,
				  fresh_copy ? command.expected_revisions.size() :
					       command.expected_revisions.capacity(),
				  sizeof(critical_expected_revision)) &&
	       birth_current_add(value, fresh_copy ? command.payload.size() :
						     command.payload.capacity()) &&
	       birth_current_add(value, fresh_copy ? command.accounting_intent.size() :
						     command.accounting_intent.capacity());
}

bool birth_current_envelope_heap(size_t &value, const critical_native_recovery_envelope &envelope,
				 bool fresh_copy = false) noexcept
{
	return birth_current_command_heap(value, envelope.command, fresh_copy) &&
	       birth_current_add(value, fresh_copy ? envelope.attachment.size() :
						     envelope.attachment.capacity());
}

bool birth_current_context_heap(size_t &value,
				const native_mobile_birth_recovery_context &progress) noexcept
{
	if (!birth_current_rows(value, progress.items.capacity(),
				sizeof(native_mobile_birth_recovery_item)))
		return false;
	for (const auto &item : progress.items)
		if (!birth_current_rows(value, item.effects.capacity(),
					sizeof(native_mobile_birth_recovery_effect)))
			return false;
	return true;
}

bool birth_current_text_heap(size_t &value, const std::string &text) noexcept
{
	return text.capacity() <= 15 ||
	       (text.capacity() != SIZE_MAX && birth_current_add(value, text.capacity() + 1));
}

bool birth_current_image_heap(size_t &value, const quest_mobile_native_image &image) noexcept
{
	if (!birth_current_rows(value, image.items.capacity(), sizeof(player_item_snapshot)))
		return false;
	for (const auto &item : image.items)
	{
		if (!birth_current_text_heap(value, item.name) ||
		    !birth_current_text_heap(value, item.short_description) ||
		    !birth_current_text_heap(value, item.description) ||
		    !birth_current_text_heap(value, item.action_description) ||
		    !birth_current_rows(value, item.dynamic_affects.capacity(),
					sizeof(player_item_dynamic_affect_snapshot)) ||
		    !birth_current_rows(value, item.extra_descriptions.capacity(),
					sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &description : item.extra_descriptions)
			if (!birth_current_text_heap(value, description.keyword) ||
			    !birth_current_text_heap(value, description.description) ||
			    !birth_current_rows(value, description.spell_ids.capacity(),
						sizeof(int32_t)))
				return false;
	}
	return true;
}

bool birth_current_recipes_heap(
	size_t &value, const std::vector<native_mobile_birth_item_recipe> &recipes) noexcept
{
	if (!birth_current_rows(value, recipes.capacity(), sizeof(native_mobile_birth_item_recipe)))
		return false;
	for (const auto &recipe : recipes)
		if (!birth_current_rows(value, recipe.libraries.capacity(),
					sizeof(native_mobile_birth_library_recipe)))
			return false;
	return true;
}

bool birth_current_stock_heap(size_t &value, const std::vector<original_item> &stock) noexcept
{
	if (!birth_current_rows(value, stock.capacity(), sizeof(original_item)))
		return false;
	for (const auto &item : stock)
		if (!birth_current_rows(value, item.effects.capacity(),
					sizeof(quest_mobile_native_item_effect)))
			return false;
	return true;
}

bool birth_current_new_body_heap(size_t &value, const original_birth &body) noexcept
{
	return birth_current_add(value, sizeof(body)) &&
	       birth_current_text_heap(value, body.ordinary_flat_recovery_root) &&
	       birth_current_image_heap(value, body.image) &&
	       birth_current_recipes_heap(value, body.recipes) &&
	       birth_current_command_heap(value, body.command) &&
	       birth_current_add(value, body.canonical.capacity()) &&
	       (!body.shared_checkpoint ||
		(birth_current_add(value, sizeof(*body.shared_checkpoint)) &&
		 birth_current_add(value, body.shared_checkpoint->capacity()))) &&
	       birth_current_rows(value, body.current_custody.capacity(),
				  sizeof(item_ownership_runtime_entry)) &&
	       birth_current_stock_heap(value, body.stock) &&
	       birth_current_envelope_heap(value, body.envelope) &&
	       birth_current_context_heap(value, body.recovery);
}
}

bool quest_mobile_native_birth_owner::current_retained_storage_bytes(size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)output;
	errno = ENOTSUP;
	return false;
#else
	if (!output || !nevent_is_game_thread())
	{
		errno = EINVAL;
		return false;
	}
	if (sizeof(void *) != 8 || sizeof(size_t) != 8)
	{
		errno = ENOTSUP;
		return false;
	}
	const int saved_errno = errno;
	errno = 0;
	size_t bytes = quest_mobile_native_birth_ordinary_execution_lease::fixed_storage_bytes();
	// These actual module objects remain resident even when a reset is inactive.
	// Aliases in actors/locals point to the separately observed actual owner.
	if (!birth_current_add(bytes, sizeof(births)) ||
	    !birth_current_add(bytes, sizeof(deferred)) ||
	    !birth_current_add(bytes, sizeof(reset_invocation)) ||
	    !birth_current_add(bytes, sizeof(reset_zone_rnum)) ||
	    !birth_current_add(bytes, sizeof(current_birth)) ||
	    !birth_current_add(bytes, sizeof(replay_ready) + sizeof(reset_in_progress) +
					      sizeof(deferred_overflow)) ||
	    !birth_current_add(bytes, sizeof(reset_dispatch)) ||
	    !birth_current_text_heap(bytes, reset_dispatch.selected_flat_root) ||
	    !birth_current_rows(bytes, births.capacity(), sizeof(births[0])) ||
	    !birth_current_rows(bytes, deferred.capacity(), sizeof(deferred[0])))
	{
		if (!errno)
			errno = EOVERFLOW;
		return false;
	}
	const bool literal_owned =
		item_native_quest_global_budget_scope_owner::literal_pool_owned();
	for (const auto &pointer : births)
		if (pointer)
		{
			const auto &body = *pointer;
			// Complete common owning values, actual row/string capacities and body
			// inline members. This helper reads values only; no closed-state predicate.
			if (!birth_current_new_body_heap(bytes, body))
			{
				if (!errno)
					errno = EOVERFLOW;
				return false;
			}
			if (body.npc_flat_factory_scope)
			{
				const size_t retained =
					body.npc_flat_factory_scope->retained_heap_bytes();
				if (!retained || !birth_current_add(bytes, retained))
				{
					if (!errno)
						errno = EIO;
					return false;
				}
			}
			if (body.ordinary_flat_source)
			{
				if (!body.ordinary_flat_source->state_)
				{
					errno = EIO;
					return false;
				}
				if (!birth_current_add(bytes, sizeof(*body.ordinary_flat_source)) ||
				    !birth_current_add(
					    bytes, sizeof(*body.ordinary_flat_source->state_)) ||
				    !birth_current_text_heap(
					    bytes,
					    body.ordinary_flat_source->state_->selected_root))
				{
					if (!errno)
						errno = EOVERFLOW;
					return false;
				}
			}
			if (literal_owned)
			{
				size_t actual_affect_heap = 0, mobile_private = 0;
				if (!body.mobile.shared_shopkeeper_affect_retained_bytes(
					    &actual_affect_heap) ||
				    !body.mobile.retained_bytes_excluding_mobile_pool(
					    &mobile_private) ||
				    !birth_current_add(bytes, actual_affect_heap) ||
				    !birth_current_add(bytes, mobile_private))
				{
					if (!errno)
						errno = EIO;
					return false;
				}
			}
			else
			{
				// Preserve the original unpaired native reservation partition. The settled
				// charge/default behavior is not changed by this additive pure observation.
				if (!body.mobile.shared_shopkeeper_affect_charge(&bytes) ||
				    (!body.mobile_consumed &&
				     !birth_current_add(bytes, body.mobile_bytes)))
				{
					if (!errno)
						errno = EIO;
					return false;
				}
			}
			const auto writer_heap =
				[&](const original_birth_checkpoint *writer) noexcept
			{
				return !writer ||
				       (birth_current_add(bytes, sizeof(*writer)) &&
					birth_current_envelope_heap(bytes, writer->expected) &&
					birth_current_envelope_heap(bytes, writer->successor) &&
					birth_current_context_heap(bytes, writer->context));
			};
			if ((body.ack_successor &&
			     (!birth_current_add(bytes, sizeof(*body.ack_successor)) ||
			      !birth_current_envelope_heap(bytes, *body.ack_successor))) ||
			    !writer_heap(body.checkpoint.get()))
			{
				if (!errno)
					errno = EOVERFLOW;
				return false;
			}
			for (const auto &writer : body.returned_writers)
				if (!writer_heap(writer.get()))
				{
					if (!errno)
						errno = EOVERFLOW;
					return false;
				}
			if (body.chosen_context &&
			    (!birth_current_add(bytes, sizeof(*body.chosen_context)) ||
			     !birth_current_context_heap(bytes, *body.chosen_context)))
			{
				if (!errno)
					errno = EOVERFLOW;
				return false;
			}
			const size_t bindings = body.bindings.retained_bytes();
			if (!bindings || bindings < sizeof(body.bindings) ||
			    !birth_current_add(bytes, bindings - sizeof(body.bindings)))
			{
				if (!errno)
					errno = EIO;
				return false;
			}
			for (const auto &item : body.stock)
				if (item.stage)
				{
					size_t stage = 0;
					if (literal_owned && item.stage->is_flat_factory())
					{
						if (!item.stage
							     ->retained_bytes_excluding_literal_pools(
								     &stage))
						{
							if (!errno)
								errno = EIO;
							return false;
						}
					}
					else
						stage = item.stage->retained_bytes();
					if (!stage || !birth_current_add(bytes, stage))
					{
						if (!errno)
							errno = EIO;
						return false;
					}
				}
		}
	size_t warm = 0;
	if (!zone_reset_item_owner::warm_retained_size(&warm) || !birth_current_add(bytes, warm))
	{
		if (!errno)
			errno = EIO;
		return false;
	}
	*output = bytes;
	errno = saved_errno;
	return true;
#endif
}

bool quest_mobile_native_birth_retained_storage_bytes(size_t *output) noexcept
{
	return quest_mobile_native_birth_owner::current_retained_storage_bytes(output);
}
