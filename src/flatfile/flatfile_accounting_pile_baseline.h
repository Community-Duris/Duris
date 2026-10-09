#ifndef DURIS_FLATFILE_ACCOUNTING_PILE_BASELINE_H
#define DURIS_FLATFILE_ACCOUNTING_PILE_BASELINE_H

#include "economy/economic_baseline_adapter.h"
#include "flatfile/flatfile_accounting_coin_transaction.h"
#include "flatfile/flatfile_accounting_pile_state.h"

struct flatfile_accounting_pile_baseline_source
{
	economic_baseline_holding holding;
	economic_baseline_item item;
	flatfile_accounting_pile_state head;
};

// Capture one pre-existing native pile under the frozen authority boundary.
// The lifecycle owner must prove complete UID coverage and commit this head
// with the corresponding baseline witness and receipt before activation.
// Outputs are unchanged on failure.
unsigned int flatfile_accounting_pile_baseline_capture(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &lineage, const critical_operation_id &epoch,
	const critical_operation_id &operation_id, uint64_t uid,
	flatfile_accounting_pile_baseline_source *source, std::string *error) noexcept;

// Read one retained baseline pile under the caller's original authority lock.
// The exact original command must be selected by the current head operation ID.
// Baseline lookup authenticates the receipt, complete witness and reservations;
// native custody, active epoch and complete canonical source are checked here.
// Native revisions remain distinct from opening accounting effects (0->1).
// Empty coin_payload uses the existing native world-literal reader. No typed
// COIN endpoint/result, mutation, publication reservation or ACK is fabricated.
// Outputs are unchanged on every refusal; this value grants no cutover authority.
unsigned int flatfile_accounting_pile_baseline_read_room_locked(
    const std::string &root, const flatfile_authority_lock &lock,
    const critical_command &original, uint64_t uid, flatfile_room_coin_pile *output,
    std::string *error) noexcept;

#endif
