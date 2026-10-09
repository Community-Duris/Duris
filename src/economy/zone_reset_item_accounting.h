#ifndef ZONE_RESET_ITEM_ACCOUNTING_H
#define ZONE_RESET_ITEM_ACCOUNTING_H

#include "economy/zone_reset_item_command.h"

// Pure expected effects of the original reset command. The atomic owner must
// authenticate its invocation/slot, absent reserved outputs, actual room/season,
// source entitlement and original artifact/hook decisions before recording this
// plan with custody, literal payloads and its receipt. No source admission, SQL,
// mappings, identity issuance, native publication, recovery or ACK is supplied.
// Strong output on every refusal, including allocation/capacity failure.
economic_accounting_error zone_reset_item_accounting_compile(const critical_command &,
							     economic_accounting_plan *) noexcept;

// Original pure compiler with prospective decode/metadata/normalization storage
// admission. Outer includes command, prior output and callback context. Hold
// admitted peaks until callee temporaries die; optional retained heap excludes
// the caller's inline plan. All outputs stay unchanged on refusal; no writer,
// native mutation, source entitlement, publication or ACK authority is supplied.
// Implementation is owned by the shared contract owner, not custody storage.
economic_accounting_error
zone_reset_item_accounting_compile_bounded(const critical_command &, economic_accounting_plan *,
					   bool (*reserve_scratch_peak)(size_t, void *) noexcept,
					   void *context, size_t outer_live_scratch,
					   size_t *retained_plan_heap_bytes = nullptr) noexcept;

#endif
