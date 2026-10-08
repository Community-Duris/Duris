#include "core/prototypes.h"
#include "economy/economic_command_admission.h"
#include "player/player_snapshot_capture.h"
#include "economy/shop.h"
#include "economy/shop_trade_world_witness.h"
#include "player/inert_item_stage.h"
#include "world/object_template.h"
#include "persistence/economic_sql_shop_trade_transaction.h"
#include "persistence/critical_command_repository.h"
#include "persistence/persistence_mode.h"
#ifndef __NO_MYSQL__
#include "sql/sql_pool.h"
#endif

#include "economy/currency_transaction.h"
#include "economy/shop_trade_transaction.h"
#include "economy/shop_trade_runtime.h"
#include "item/item_ownership_runtime.h"
#include "core/utils.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string_view>
#include <utility>

namespace
{
char_data actor = {};
pc_only_data pc = {};
bool online = true;
critical_command submitted = {};
critical_completion receipt = {};
enum class boundary
{
	none,
	preflight,
	balances,
	custody,
	revision,
	exception,
	reentry
};
boundary fault = boundary::none;
unsigned int submits = 0, balance_calls = 0, custody_calls = 0, revision_calls = 0;
unsigned int callbacks = 0;
bool notified_commit = false, escaped = false, notification_reentry = false;
bool notification_exception = false;

bool check(bool value, const char *label)
{
	if (!value)
		std::fprintf(stderr, "ASSERTION FAILED: %s\n", label);
	return value;
}

shop_trade_payload payload(uint64_t uid = 300)
{
	shop_trade_payload p = {};
	p.action = shop_trade_action::buy_produced;
	p.player_pid = 42;
	p.shop_id = 7;
	p.racewar = 1;
	std::strcpy(p.account_name.data(), "synthetic-shop-account");
	p.price = 100;
	p.expected_wallet_revision = 2;
	p.expected_bank_revision = 3;
	p.expected_shop_revision = 4;
	p.selected_item_uid = p.target_root_item_uid = uid;
	p.item_count = 1;
	p.item_blob_size = 1;
	p.item_blob[0] = 0x5a;
	p.stock_item_uid = 900;
	p.expected_stock_item_revision = 6;
	p.stock_vnum = 800;
	p.items[0] = { uid, uid, 0, ITEM_TRANSFER_ABSENT_REVISION, 800, item_custody_state::absent };
	return p;
}

void notification(P_char character, bool committed, const shop_trade_result &result,
		  unsigned int error, const shop_trade_payload &original)
{
	if (!check(character == &actor, "completion actor differs"))
		std::abort();
	++callbacks;
	notified_commit = committed;
	if (committed && !check(!error && result.item_uids[0] == original.selected_item_uid,
				"completion replaced committed original result"))
		std::abort();
	if (notification_reentry)
	{
		notification_reentry = false;
		if (!check(shop_trade_transaction_submit(&actor, payload(301), notification),
			   "post-success continuation remained blocked"))
			std::abort();
	}
	if (notification_exception)
		throw std::bad_alloc();
}

critical_completion committed_result()
{
	shop_trade_result r = {};
	r.action = shop_trade_action::buy_produced;
	r.wallet = { { 0, 0, 9, 9 } };
	r.wallet_revision = 3;
	r.bank_revision = 4;
	r.shop_revision = 5;
	r.player_owner_revision = 8;
	r.counterparty_owner_revision = 6;
	r.item_count = 1;
	r.item_uids[0] = 300;
	r.item_revisions[0] = 1;
	std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> encoded = {};
	if (!shop_trade_command_encode_result(r, &encoded))
		std::abort();
	critical_completion c = {};
	c.operation_id = submitted.operation_id;
	c.outcome = critical_apply_outcome::applied;
	c.durable_revision = 5;
	c.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), c.result_payload.begin());
	return c;
}

void deliver()
{
	try
	{
		shop_trade_transaction_handle_completions(&receipt, 1);
	}
	catch (...)
	{
		escaped = true;
	}
}
} // namespace

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	++submits;
	submitted = std::move(command);
	return critical_submit_result::accepted;
}

P_char find_player_by_pid(int pid)
{
	return online && pid == 42 ? &actor : nullptr;
}

bool currency_transaction_publish_balances(P_char character, const char *account, uint8_t racewar,
					   const currency_vector &, const currency_vector &,
					   uint64_t wallet, uint64_t bank)
{
	if (!check(character == &actor && !std::strcmp(account, "synthetic-shop-account") &&
			   racewar == 1 && wallet == 3 && bank == 4,
		   "balance projection original identity"))
		std::abort();
	++balance_calls;
	if (fault == boundary::exception)
		throw std::bad_alloc();
	if (fault == boundary::reentry)
	{
		fault = boundary::none;
		shop_trade_transaction_player_ready(&actor);
	}
	return fault != boundary::balances;
}

bool item_ownership_runtime_apply(const item_transfer_payload &p, const item_transfer_result &r)
{
	if (!check(p.selected_item_uid == 300 && p.from_owner.type == item_owner_type::system &&
			   p.to_owner.type == item_owner_type::player && r.item_count == 1 &&
			   r.to_owner_revision == 8,
		   "custody projection original identity"))
		std::abort();
	++custody_calls;
	return fault != boundary::custody;
}

bool shop_trade_runtime_can_advance(uint32_t shop, uint64_t before, uint64_t after)
{
	return shop == 7 && before == 4 && after == 5 && fault != boundary::preflight;
}

bool shop_trade_runtime_advance(uint32_t shop, uint64_t before, uint64_t after)
{
	if (!check(shop == 7 && before == 4 && after == 5, "shop revision original identity"))
		std::abort();
	++revision_calls;
	return fault != boundary::revision;
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return 2;
	const std::string_view name(argv[1]);
	actor.only.pc = &pc;
	pc.pid = 42;
	if (!check(shop_trade_transaction_submit(&actor, payload(), notification),
		   "initial submit"))
		return 1;
	receipt = committed_result();
	if (name == "preflight")
		fault = boundary::preflight;
	else if (name == "balances")
		fault = boundary::balances;
	else if (name == "custody")
		fault = boundary::custody;
	else if (name == "revision")
		fault = boundary::revision;
	else if (name == "exception")
		fault = boundary::exception;
	else if (name == "reentry")
		fault = boundary::reentry;
	else if (name == "missing_player")
		online = false;
	else if (name == "malformed")
		receipt.result_size = 1;
	else if (name == "ambiguous")
		receipt.outcome = critical_apply_outcome::ambiguous_commit;
	else if (name == "wrong_action")
		receipt.result_payload[0] = static_cast<uint8_t>(shop_trade_action::sell_store);
	else if (name == "wrong_uid")
		receipt.result_payload[112] ^= 1;
	else if (name == "invalid_disposition")
		receipt.disposition = critical_completion_disposition::never_admitted;
	else if (name == "changed_duplicate")
		fault = boundary::balances;
	else if (name == "equivalent_readback")
		fault = boundary::balances;
	else if (name == "fresh_body")
		fault = boundary::custody;
	else if (name == "notification_reentry")
		notification_reentry = true;
	else if (name == "notification_exception" || name == "notification_reentry_exception")
	{
		notification_exception = true;
		notification_reentry = name == "notification_reentry_exception";
	}
	else if (name == "many_transient_failures" || name == "duplicate_burst")
		fault = boundary::balances;
	else if (name != "success")
		return 2;
	deliver();
	bool passed = check(!escaped, "projection exception escaped retained owner");
	if (name == "success" || name == "reentry" || name == "notification_reentry")
	{
		passed &= check(callbacks == 1 && notified_commit,
				"successful original result not notified once");
		passed &= check(balance_calls == 1 && custody_calls == 1 && revision_calls == 1,
				"reentry repeated committed publication stage");
		passed &= check(shop_trade_transaction_player_busy(&actor) ==
					(name == "notification_reentry"),
				"pending-node lifetime erased continuation or retained success");
	}
	else if (notification_exception)
	{
		passed &= check(shop_trade_transaction_player_busy(&actor) && callbacks == 1 &&
					submits == (name == "notification_reentry_exception" ? 2U :
											       1U),
				"throwing notification lost original owner or continuation");
		shop_trade_transaction_player_ready(&actor);
		passed &= check(callbacks == 1 && shop_trade_transaction_player_busy(&actor),
				"uncertain notification was repeated automatically");
	}
	else if (name == "duplicate_burst")
	{
		const std::array<critical_completion, 3> duplicate = { receipt, receipt, receipt };
		shop_trade_transaction_handle_completions(duplicate.data(), duplicate.size());
		passed &= check(balance_calls == 2 && callbacks == 0 && submits == 1,
				"completion burst repeated a failed stage within one dispatch");
		fault = boundary::none;
		shop_trade_transaction_player_ready(&actor);
		passed &= check(balance_calls == 3 && callbacks == 1 &&
					!shop_trade_transaction_player_busy(&actor),
				"burst retry lost original operation");
	}
	else if (name == "equivalent_readback")
	{
		passed &= check(callbacks == 0 && shop_trade_transaction_player_busy(&actor),
				"initial failure lost original committed result");
		receipt.outcome = critical_apply_outcome::already_applied;
		receipt.attempt = 2;
		fault = boundary::none;
		deliver();
		passed &= check(callbacks == 1 && notified_commit && submits == 1 &&
					!shop_trade_transaction_player_busy(&actor),
				"equivalent committed readback could not retry original result");
	}
	else if (name == "many_transient_failures")
	{
		for (unsigned int attempt = 0; attempt < 20; ++attempt)
			shop_trade_transaction_player_ready(&actor);
		fault = boundary::none;
		shop_trade_transaction_player_ready(&actor);
		passed &= check(
			balance_calls == 22 && callbacks == 1 && submits == 1 &&
				!shop_trade_transaction_player_busy(&actor),
			"repaired dependency could not resume one-attempt-per-call publication");
	}
	else
	{
		passed &=
			check(shop_trade_transaction_player_busy(&actor) && callbacks == 0 &&
				      submits == 1,
			      "postcommit failure erased original owner or reported failed debit");
		passed &= check(!shop_trade_transaction_submit(&actor, payload(301), notification),
				"postcommit failure allowed another purchase");
		if (name == "changed_duplicate")
		{
			const auto original = receipt;
			receipt.result_payload[8] ^= 1;
			fault = boundary::none;
			deliver();
			passed &=
				check(shop_trade_transaction_player_busy(&actor) && callbacks == 0,
				      "conflicting completion replaced original committed receipt");
			receipt = original;
		}
		else if (name != "malformed" && name != "ambiguous" && name != "wrong_action" &&
			 name != "wrong_uid" && name != "invalid_disposition")
		{
			fault = boundary::none;
			online = true;
			if (name == "fresh_body")
				++actor.runtime_id;
			shop_trade_transaction_player_ready(&actor);
			passed &= check(callbacks == 1 && notified_commit && submits == 1 &&
						!shop_trade_transaction_player_busy(&actor),
					"bounded retry lost original result");
			if (name == "custody" || name == "revision")
				passed &= check(balance_calls == 1,
						"retry repeated completed balance projection");
			if (name == "revision")
				passed &= check(custody_calls == 1,
						"retry repeated completed custody projection");
			if (name == "fresh_body")
				passed &= check(balance_calls == 2,
						"fresh body missed original balance projection");
		}
	}
	std::printf(
		"CASE %s result=%s submits=%u balance_calls=%u custody_calls=%u revision_calls=%u callbacks=%u escaped=%d\n",
		argv[1], passed ? "pass" : "fail", submits, balance_calls, custody_calls,
		revision_calls, callbacks, escaped);
	shop_trade_transaction_reset_for_tests();
	return passed ? 0 : 1;
}

// These original v5 component cases never construct a native preparation or cold
// restored owner. New retained call graphs must not grant that missing authority.
// Any unexpected call fails the fixture immediately, including empty staging.
namespace
{
[[noreturn]] void unavailable_shop_native_capability(const char *name) noexcept
{
	std::fprintf(stderr, "UNEXPECTED outside-scope SHOP capability: %s\n", name);
	std::abort();
}
}
bool player_save_restored_publication_owner::publish_shop(
	const critical_command &, const critical_completion &,
	bool (*)(const critical_command &, const critical_completion &, void *) noexcept,
	void *) noexcept
{
	unavailable_shop_native_capability("player_save_restored_publication_owner::publish_shop");
}
bool nevent_is_game_thread()
{
	unavailable_shop_native_capability("nevent_is_game_thread");
}
P_char find_character_by_runtime_id(uint64_t)
{
	unavailable_shop_native_capability("find_character_by_runtime_id");
}
const char *get_account_name_safe(P_char)
{
	unavailable_shop_native_capability("*get_account_name_safe");
}
bool economic_gameplay_authority::active()
{
	unavailable_shop_native_capability("economic_gameplay_authority::active");
}
bool economic_gameplay_authority::active_regular_sql()
{
	unavailable_shop_native_capability("economic_gameplay_authority::active_regular_sql");
}
bool economic_gameplay_authority::observe_shop_checkpoint(
	uint32_t, std::string_view, uint8_t, economic_shop_checkpoint_projection *) noexcept
{
	unavailable_shop_native_capability("economic_gameplay_authority::observe_shop_checkpoint");
}
economic_accounting_error economic_gameplay_authority::prepare_shop_trade(critical_command *)
{
	unavailable_shop_native_capability("economic_gameplay_authority::prepare_shop_trade");
}
bool economic_shop_trade_admission_available() noexcept
{
	unavailable_shop_native_capability("economic_shop_trade_admission_available");
}
persistence_mode persistence_mode_get()
{
	unavailable_shop_native_capability("persistence_mode_get");
}
bool obj_capture_container_shell_weight(P_obj, int32_t *)
{
	unavailable_shop_native_capability("obj_capture_container_shell_weight");
}
bool obj_to_obj_shop_frozen_weight(P_obj, P_obj, const shop_trade_destination_weight &)
{
	unavailable_shop_native_capability("obj_to_obj_shop_frozen_weight");
}
bool shop_item_runtime_capture_keeper_literal(char_data *,
					      std::vector<player_item_snapshot> *) noexcept
{
	unavailable_shop_native_capability("shop_item_runtime_capture_keeper_literal");
}
bool player_save_pipeline_restore_sql_shop_obligation(const critical_command &)
{
	unavailable_shop_native_capability("player_save_pipeline_restore_sql_shop_obligation");
}
player_literal_inventory_state
player_save_pipeline_shop_checkpoint_begin(P_char, P_obj, int, player_shop_checkpoint_token *)
{
	unavailable_shop_native_capability("player_save_pipeline_shop_checkpoint_begin");
}
player_literal_inventory_state
player_save_pipeline_shop_checkpoint_poll(const player_shop_checkpoint_token &, P_char,
					  player_shop_checkpoint_stage *)
{
	unavailable_shop_native_capability("player_save_pipeline_shop_checkpoint_poll");
}
bool player_save_pipeline_shop_checkpoint_hold(const player_shop_checkpoint_token &,
					       const critical_operation_id &)
{
	unavailable_shop_native_capability("player_save_pipeline_shop_checkpoint_hold");
}
bool player_save_pipeline_shop_checkpoint_release(const player_shop_checkpoint_token &,
						  const critical_operation_id &)
{
	unavailable_shop_native_capability("player_save_pipeline_shop_checkpoint_release");
}
bool player_save_pipeline_shop_checkpoint_cancel(const player_shop_checkpoint_token &)
{
	unavailable_shop_native_capability("player_save_pipeline_shop_checkpoint_cancel");
}
bool player_save_shop_checkpoint_owner::observe_held(const player_shop_checkpoint_token &, P_char,
						     const critical_operation_id &,
						     player_shop_checkpoint_stage *,
						     std::vector<player_item_snapshot> *) noexcept
{
	unavailable_shop_native_capability("player_save_shop_checkpoint_owner::observe_held");
}
critical_submit_result
player_save_shop_checkpoint_owner::submit_owned(const player_shop_checkpoint_token &,
						critical_command, bool *) noexcept
{
	unavailable_shop_native_capability("player_save_shop_checkpoint_owner::submit_owned");
}
bool player_save_shop_checkpoint_owner::original_held_body(const player_shop_checkpoint_token &,
							   const critical_command &,
							   std::vector<player_item_snapshot> *,
							   player_shop_checkpoint_stage *) noexcept
{
	unavailable_shop_native_capability("player_save_shop_checkpoint_owner::original_held_body");
}
shop_trade_preparation_state
shop_trade_native_checkpoint_owner::attempt(const shop_trade_preparation_token &, P_char, P_char,
					    P_obj, P_obj, P_obj) noexcept
{
	unavailable_shop_native_capability("shop_trade_native_checkpoint_owner::attempt");
}
bool shop_trade_world_witness_observe(const shop_trade_world_expectation &,
				      shop_trade_world_witness *) noexcept
{
	unavailable_shop_native_capability("shop_trade_world_witness_observe");
}
bool shop_trade_world_player_values_supported(std::span<const player_item_snapshot>) noexcept
{
	unavailable_shop_native_capability("shop_trade_world_player_values_supported");
}
bool shop_trade_world_expected_player_order(const shop_trade_payload &, char_data *, obj_data *,
					    obj_data *, const std::vector<player_item_snapshot> &,
					    std::vector<player_item_snapshot> *) noexcept
{
	unavailable_shop_native_capability("shop_trade_world_expected_player_order");
}
bool shop_trade_world_expected_keeper_order(const shop_trade_payload &, char_data *, obj_data *,
					    const std::vector<player_item_snapshot> &,
					    std::vector<player_item_snapshot> *) noexcept
{
	unavailable_shop_native_capability("shop_trade_world_expected_keeper_order");
}
bool shop_trade_world_cold_observe(const shop_trade_world_cold_request &,
				   shop_trade_world_cold_observation *) noexcept
{
	unavailable_shop_native_capability("shop_trade_world_cold_observe");
}
void publish_account_bank_balances_revision(const char *, int, const AccountBankBalances *,
					    uint64_t)
{
	unavailable_shop_native_capability("publish_account_bank_balances_revision");
}

#ifndef __NO_MYSQL__
critical_apply_result
critical_command_repository_verify_shop_trade_in_transaction(MYSQL *,
							     const critical_command &) noexcept
{
	unavailable_shop_native_capability(
		"critical_command_repository_verify_shop_trade_in_transaction");
}
unsigned int economic_sql_shop_trade_lock_publication(
	MYSQL *, const critical_command &, const critical_completion &,
	std::span<const player_item_snapshot>, economic_sql_shop_trade_publication *) noexcept
{
	unavailable_shop_native_capability("economic_sql_shop_trade_lock_publication");
}
unsigned int
economic_sql_shop_trade_lock_publication(MYSQL *, const critical_command &,
					 const critical_completion &,
					 economic_sql_shop_trade_publication *) noexcept
{
	unavailable_shop_native_capability("economic_sql_shop_trade_lock_publication");
}
unsigned int
economic_sql_shop_trade_lock_never_admitted_before(MYSQL *, const critical_command &,
						   economic_sql_shop_trade_publication *) noexcept
{
	unavailable_shop_native_capability("economic_sql_shop_trade_lock_never_admitted_before");
}
bool shop_trade_current_runtime_owner::publish(st_mysql *, const shop_trade_payload &,
					       const economic_sql_shop_trade_publication &) noexcept
{
	unavailable_shop_native_capability("shop_trade_current_runtime_owner::publish");
}
const object_template *find_recovery_object_template(int) noexcept
{
	unavailable_shop_native_capability("*find_recovery_object_template");
}
bool shop_trade_original_item_stage::prepare(const object_template &, const player_item_snapshot &,
					     shop_trade_original_item_stage &) noexcept
{
	unavailable_shop_native_capability("shop_trade_original_item_stage::prepare");
}
bool shop_trade_original_item_stage::reload_step(P_obj, const object_template &, unsigned int,
						 shop_trade_original_reload_effect &) noexcept
{
	unavailable_shop_native_capability("shop_trade_original_item_stage::reload_step");
}
bool shop_trade_original_item_stage::proclib_probe(P_obj, const object_template &, size_t,
						   shop_trade_original_reload_effect &) noexcept
{
	unavailable_shop_native_capability("shop_trade_original_item_stage::proclib_probe");
}
void shop_trade_original_item_stage::reset() noexcept
{
	unavailable_shop_native_capability("shop_trade_original_item_stage::reset");
}
shop_trade_original_item_stage::~shop_trade_original_item_stage() noexcept
{
	unavailable_shop_native_capability(
		"shop_trade_original_item_stage::~shop_trade_original_item_stage");
}
shop_trade_original_item_stage::shop_trade_original_item_stage(
	shop_trade_original_item_stage &&) noexcept
{
	unavailable_shop_native_capability(
		"shop_trade_original_item_stage::shop_trade_original_item_stage");
}
shop_trade_original_item_stage &
shop_trade_original_item_stage::operator=(shop_trade_original_item_stage &&) noexcept
{
	unavailable_shop_native_capability("&shop_trade_original_item_stage::operator=");
}
bool shop_trade_original_procedure_binding_stage::prepare(
	std::span<const P_obj>, std::span<const player_item_snapshot>,
	shop_trade_original_procedure_binding_stage &) noexcept
{
	unavailable_shop_native_capability("shop_trade_original_procedure_binding_stage::prepare");
}
bool shop_trade_original_procedure_binding_stage::valid() const noexcept
{
	unavailable_shop_native_capability("shop_trade_original_procedure_binding_stage::valid");
}
void shop_trade_original_procedure_binding_stage::commit_unchecked() noexcept
{
	unavailable_shop_native_capability(
		"shop_trade_original_procedure_binding_stage::commit_unchecked");
}
proclib_recovery_chain_stage::~proclib_recovery_chain_stage() noexcept
{
	unavailable_shop_native_capability(
		"proclib_recovery_chain_stage::~proclib_recovery_chain_stage");
}
proclib_recovery_chain_stage::proclib_recovery_chain_stage(proclib_recovery_chain_stage &&) noexcept
{
	unavailable_shop_native_capability(
		"proclib_recovery_chain_stage::proclib_recovery_chain_stage");
}
proclib_recovery_chain_stage &
proclib_recovery_chain_stage::operator=(proclib_recovery_chain_stage &&) noexcept
{
	unavailable_shop_native_capability("&proclib_recovery_chain_stage::operator=");
}
MYSQL *sql_pool_acquire(void)
{
	unavailable_shop_native_capability("*sql_pool_acquire");
}
void sql_pool_discard_connection(MYSQL *)
{
	unavailable_shop_native_capability("sql_pool_discard_connection");
}
void sql_pool_release(MYSQL *)
{
	unavailable_shop_native_capability("sql_pool_release");
}
bool sql_pool_retire_owned_connection(MYSQL *)
{
	unavailable_shop_native_capability("sql_pool_retire_owned_connection");
}
MYSQL *sql_pool_replace_connection(MYSQL *)
{
	unavailable_shop_native_capability("*sql_pool_replace_connection");
}
#endif

#ifndef __NO_MYSQL__
// Unavailable world roots are never a census. The guarded observer above aborts.
struct shop_data *shop_index = nullptr;
int number_of_shops = 0;
P_index obj_index = nullptr;
int top_of_objt = 0;
P_obj object_list = nullptr;
#endif
bool shop_trade_cold_native_step_execute(P_char, P_char, P_obj, P_obj, const shop_trade_payload &,
					 shop_trade_cold_native_effect &) noexcept
{
	unavailable_shop_native_capability("shop_trade_cold_native_step_execute");
}
player_snapshot_capture_result
player_item_snapshot_tree_capture_literal(P_obj, std::vector<player_item_snapshot> *, size_t *)
{
	unavailable_shop_native_capability("player_item_snapshot_tree_capture_literal");
}

shop_trade_payload_build_result shop_trade_runtime_build_accounted_payload(
	P_char, P_char, P_obj, P_obj, P_obj, uint32_t, shop_trade_action, int64_t,
	const player_shop_checkpoint_stage &, uint64_t, shop_trade_payload *)
{
	unavailable_shop_native_capability("shop_trade_runtime_build_accounted_payload");
}

bool item_ownership_runtime_lookup(uint64_t, item_ownership_runtime_entry *)
{
	unavailable_shop_native_capability("item_ownership_runtime_lookup");
}
