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

// Prospective storage admission beside the original codecs and metadata proof.
// Caller owns inputs, old output and context in outer_live; retain admitted
// peaks through the original call and output transfer. Unsupported allocation
// policy refuses. These helpers grant no writer, source or execution authority.
economic_accounting_error
economic_intent_decode_bounded(const std::span<const uint8_t> &encoded, economic_frozen_intent *,
			       bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
			       size_t outer_live) noexcept;
economic_accounting_error
economic_intent_plan_metadata_bounded(const critical_command &, const economic_frozen_intent &,
				      economic_plan_metadata *,
				      bool (*reserve_scratch_peak)(size_t, void *) noexcept,
				      void *context, size_t outer_live) noexcept;

economic_accounting_error
economic_intent_verify_binding_bounded(const critical_command &, const economic_frozen_intent &,
				       bool (*reserve_scratch_peak)(size_t, void *) noexcept,
				       void *context, size_t outer_live) noexcept;

// Genuine prospective encode/freeze companions. Caller includes input storage,
// old outputs and inline outputs in outer_live and retains the admitted absolute
// simultaneous peak through transfer. Full original metadata/binding/domain
// and intent wire semantics remain authoritative; no source/execution authority.
// Fresh copy/reserve/prepend requests require GCC13 libstdc++ C++11 ABI. Strong
// outputs on semantic, budget, unsupported-policy and allocation refusal.
economic_accounting_error economic_intent_encode_bounded(const economic_frozen_intent &,
							 std::vector<uint8_t> *,
							 bool (*)(size_t, void *) noexcept, void *,
							 size_t outer_live) noexcept;
economic_accounting_error economic_intent_freeze_bounded(const critical_command &,
							 const economic_admission_facts &,
							 std::vector<uint8_t> *,
							 bool (*)(size_t, void *) noexcept, void *,
							 size_t outer_live) noexcept;

// Full original freeze/domain-tagged digest/encode closure with genuine fixed
// SHA context and actual vector COPY/front-insert/constructor request ownership.
// Prior bounded/original methods stay byte exact. Caller owns authentic inputs,
// prior encoded output and all sibling state. Strong output; no authority gate.
economic_accounting_error
economic_intent_freeze_fixed_bounded(const critical_command &, const economic_admission_facts &,
				     std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
				     void *context, size_t outer_live) noexcept;

#endif
