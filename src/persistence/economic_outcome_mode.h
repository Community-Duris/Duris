#ifndef DURIS_ECONOMIC_OUTCOME_MODE_H
#define DURIS_ECONOMIC_OUTCOME_MODE_H

// Freeze exactly one startup choice before callers/workers. Config key
// DURIS_ACCOUNTING_MODE accepts "off" (default) or "ram". Empty/unknown values
// fail startup. ECONOMIC_OUTCOME_JOURNAL_DIR is the explicit outcome locator,
// independent of ordinary critical/save journals. This is a request, never ACK authority.
// An existing acknowledged/uncertain namespace refuses ordinary OFF startup;
// its actual controlled recovery/drain must complete before changing modes.
bool economic_outcome_mode_configure(const char *journal_directory) noexcept;
bool economic_outcome_accounting_requested() noexcept;
// Check the real DB-selected legacy policy after its boot selection. A clean
// OFF request conflicting with an active selected lineage or any commissioned
// RAM stream is a boot refusal even if the old journal directory was removed
// from configuration. Merely present empty additive tables do not select RAM.
bool economic_outcome_mode_validate_selected_policy() noexcept;

#endif
