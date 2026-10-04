#ifndef DURIS_COLLECTOR_TRANSACTION_H
#define DURIS_COLLECTOR_TRANSACTION_H

#include "economy/collector_command.h"
#include "economy/economic_accounting_types.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_outbox.h"
#include "core/structs.h"

#include <cstddef>

constexpr size_t COLLECTOR_PENDING_MAX = 128;

// committed reports the durable command outcome. A nonzero error on a committed
// callback means local publication needs recovery; it never turns a committed
// custody or wallet mutation into a rejected operation.
using collector_completion_fn = void (*)(P_char character, bool committed,
					 const collector_command_result &result,
					 unsigned int error_code,
					 const collector_command_payload &payload);

// Local effect bookkeeping only: neither a native receipt nor save authority.
// A started materializer which did not return success stays unresolved; final
// UID/location alone cannot prove the handler's auxiliary effects completed.
struct collector_purchase_publication_state
{
	bool materializer_started = false;
	bool materializer_returned = false;
	bool receipt_conflict = false;

    private:
	friend class collector_purchase_publication_owner;
	bool bound_ = false;
	critical_operation_id operation_ = {};
	std::array<uint8_t, 32> command_digest_ = {};
	critical_completion original_ = {};
};
using collector_purchase_effect_fn = bool (*)(P_char, const collector_command_result &,
					      const collector_command_payload &,
					      collector_purchase_publication_state &);

// Only the named service restoration entry may choose the existing native effect
// and post-ACK notification. No public callback registration or receipt synthesis.
class collector_purchase_cold_restore_owner
{
	friend bool collector_service_restore_replayed_purchase(const critical_command &) noexcept;
	static bool restore(const critical_command &, collector_purchase_effect_fn,
			    collector_completion_fn) noexcept;
};

bool collector_transaction_submit_purchase_identified(P_char character,
						      const critical_operation_id &operation_id,
						      const collector_command_payload &payload,
						      const collector::record &original_listing,
						      collector_purchase_effect_fn effect,
						      collector_completion_fn notify);

// Accounted purchase domain owner. Shared save admission and private reserved
// ACK remain in the pipeline; the SQL native proof is implemented separately.
// Preparation freezes the existing accounted intent with a strong guarantee.
// Submission installs the original save hold before coordinator admission;
// definite refusal releases it, uncertainty preserves it and the original ID.
economic_accounting_error collector_purchase_prepare_accounted(critical_command *,
							       const collector::record &);
critical_submit_result collector_purchase_submit_for_publication(critical_command);
// Own current/historical backend/session/save proof through callback and ACK.
// True means exact original guarded ACK AND save-hold release completed. Every
// attempt revalidates, including ACK retry. A rejected receipt invokes no effect
// callback and needs native no-effect proof. No raw proof is exported as authority.
// Check state.receipt_conflict AFTER the effect callback and BEFORE guarded ACK:
// callback reentry may have delivered a contradictory sealed completion.
// Existing canonical never_admitted instead releases the original hold with
// no callback/native mutation/ACK, after validating its original disposition.
bool collector_purchase_publication_attempt(const critical_command &, const critical_completion &,
					    uint64_t actor_runtime_id,
					    collector_purchase_publication_state &,
					    collector_purchase_effect_fn);

bool collector_transaction_submit(
	P_char character, const collector_command_payload &payload,
	collector_completion_fn completion,
	critical_deadline_class deadline = critical_deadline_class::interactive);
bool collector_transaction_submit_identified(
	P_char character, const critical_operation_id &operation_id,
	const collector_command_payload &payload, collector_completion_fn completion,
	critical_deadline_class deadline = critical_deadline_class::interactive);
bool collector_transaction_submit_background(const collector_command_payload &payload,
					     collector_completion_fn completion);
bool collector_transaction_submit_background_identified(const critical_operation_id &operation_id,
							const collector_command_payload &payload,
							collector_completion_fn completion);
void collector_transaction_handle_completions(const critical_completion *completions, size_t count);
void collector_transaction_player_ready(P_char character);
bool collector_transaction_player_busy(P_char character);
// A listing may have at most one command between durable submission and
// game-thread publication. This also protects due work from ephemeral lease
// loss when a periodic authoritative cache snapshot rebuilds the runtime.
bool collector_transaction_listing_busy(uint64_t listing);
// Holds the live object graph stable from collection submission through game-thread
// publication, including the interval after the coordinator releases its worker fence.
bool collector_transaction_item_busy(uint64_t item_uid);
critical_outbox_delivery_result
collector_transaction_outbox_delivery(const critical_outbox_record &record, void *context);
void collector_transaction_publish_outbox(void);
void collector_transaction_reset_for_tests(void);

#endif
