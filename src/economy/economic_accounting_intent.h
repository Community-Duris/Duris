#ifndef DURIS_ECONOMIC_ACCOUNTING_INTENT_H
#define DURIS_ECONOMIC_ACCOUNTING_INTENT_H

#include "economy/economic_accounting_plan.h"

constexpr size_t ECONOMIC_INTENT_HEADER_BYTES = 256;
constexpr size_t ECONOMIC_INTENT_MAX_FACT_BYTES =
	ECONOMIC_ACCOUNTING_MAX_INTENT_BYTES - ECONOMIC_INTENT_HEADER_BYTES;

// Values fixed by admission, never pointers or current balances. Writer-specific
// typed adapters own the facts encoding and must verify its semantics/authority.
struct economic_admission_facts
{
	economic_operation_metadata metadata;
	uint16_t facts_version = 1;
	std::vector<uint8_t> facts;
};

struct economic_frozen_intent
{
	economic_admission_facts admission;
	economic_digest command_binding = {};
	economic_digest domain_digest = {};
};

// These are structural codecs, not a source entitlement or writer capability.
// All output arguments remain unchanged on failure.
economic_accounting_error economic_intent_freeze(const critical_command &command,
						 const economic_admission_facts &facts,
						 std::vector<uint8_t> *encoded);
economic_accounting_error economic_intent_encode(const economic_frozen_intent &intent,
						 std::vector<uint8_t> *encoded);
economic_accounting_error economic_intent_decode(std::span<const uint8_t> encoded,
						 economic_frozen_intent *intent);
economic_accounting_error economic_intent_verify_binding(const critical_command &command,
							 const economic_frozen_intent &intent);
economic_accounting_error economic_intent_digest(const economic_frozen_intent &intent,
						 economic_digest *digest);
economic_accounting_error economic_intent_plan_metadata(const critical_command &command,
							const economic_frozen_intent &intent,
							economic_plan_metadata *metadata);

#endif
