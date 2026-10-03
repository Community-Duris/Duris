#ifndef DURIS_FLATFILE_ACCOUNTING_PILE_BASELINE_H
#define DURIS_FLATFILE_ACCOUNTING_PILE_BASELINE_H

#include "economy/economic_baseline_adapter.h"
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

#endif
