#ifndef DURIS_FLATFILE_ACCOUNTING_PILE_STATE_H
#define DURIS_FLATFILE_ACCOUNTING_PILE_STATE_H

#include "economy/economic_accounting_plan.h"
#include "flatfile/flatfile_accounting_store.h"

struct flatfile_accounting_pile_state
{
	economic_account_key account;
	critical_operation_id epoch;
	critical_operation_id operation_id;
	economic_coin_vector balance = {};
	uint64_t item_revision = 0;
	bool retired = false;
};

// The UID is the pile account's authority ID. A retired head is retained so
// that the same item lifetime can never be created again. Both calls require
// the authority lock; stage only appends an image for the caller's root commit.
flatfile_accounting_status
flatfile_accounting_pile_state_read(const std::string &root, const flatfile_authority_lock &lock,
				    uint64_t uid, flatfile_accounting_pile_state *state,
				    std::string *error);
// Structural boot census, including retired heads and earlier lineages. The
// existing boot owner supplies its existing census bound. Malformed recognizable
// names or damaged bodies refuse; absence is not successful empty history.
// This is not a source/native proof or permission to restore any pile.
// Preserve output on every failure; caller holds the original authority cut.
flatfile_accounting_status flatfile_accounting_pile_state_list(
	const std::string &root, const flatfile_authority_lock &lock, size_t maximum,
	std::vector<flatfile_accounting_pile_state> *states, std::string *error);
// Capture a pre-existing native pile with its current item revision as part
// of the opening baseline. The caller commits this image with its witness.
flatfile_accounting_status flatfile_accounting_pile_state_stage_baseline(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_pile_state &state,
	std::vector<flatfile_authority_operation> *operations, std::string *error);
flatfile_accounting_status flatfile_accounting_pile_state_stage(
	const std::string &root, const flatfile_authority_lock &lock,
	const economic_account_effect &effect, const critical_operation_id &epoch,
	const critical_operation_id &operation_id, bool retired,
	std::vector<flatfile_authority_operation> *operations, std::string *error);

// Passive prospective scratch admission; same recovered lock and original head proof.
// Input/prior output storage belongs to outer; no diagnostics allocate.
flatfile_accounting_status
flatfile_accounting_pile_state_read_bounded(const std::string &, const flatfile_authority_lock &,
					    uint64_t, flatfile_accounting_pile_state *,
					    flatfile_scratch_reserve_fn, void *, size_t) noexcept;

#endif
