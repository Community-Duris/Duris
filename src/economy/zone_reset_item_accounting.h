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

#endif
