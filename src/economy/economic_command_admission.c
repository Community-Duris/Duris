#include "economy/economic_command_admission.h"
#include "economy/economic_currency_adapter.h"

#include <new>

namespace
{
uint64_t little_u64(std::span<const uint8_t> input, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= uint64_t(input[offset + byte]) << (8 * byte);
	return value;
}
}

bool economic_command_admission_supported(const critical_command &command) noexcept
{
	using error = economic_accounting_error;
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return false;
	try
	{
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) != error::ok)
			return false;
		const auto &meta = intent.admission.metadata;
		const auto facts = std::span<const uint8_t>(intent.admission.facts);
		// This projection is used only to regenerate the original immutable
		// intent. Execution always receives the complete schema-2 command.
		auto admission = command;
		admission.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		admission.accounting_intent.clear();
		std::vector<uint8_t> expected;
		if (command.type == critical_command_type::account_bank)
		{
			if (facts.size() != ECONOMIC_BANK_FACT_BYTES)
				return false;
			const economic_account_key wallet = { meta.lineage,
							      economic_account_kind::wallet,
							      little_u64(facts, 0), 0 };
			const economic_account_key bank = { meta.lineage,
							    economic_account_kind::bank,
							    little_u64(facts, 8),
							    little_u64(facts, 16) };
			if (economic_bank_transfer_intent(admission, meta.epoch, wallet, bank,
							  &expected) != error::ok)
				return false;
		}
		else
			return false;
		return expected == command.accounting_intent;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
