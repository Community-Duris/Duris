#include "economy/economic_baseline_command.h"
#include <algorithm>
#include <new>
#include <openssl/sha.h>

namespace
{
using error = economic_accounting_error;
void put(std::vector<uint8_t> &bytes, size_t offset, uint64_t value, size_t width)
{
	for (size_t byte = 0; byte < width; ++byte)
		bytes[offset + byte] = static_cast<uint8_t>(value >> (8 * byte));
}
}

economic_accounting_error
economic_baseline_command_build(const economic_prepared_baseline &prepared,
				uint64_t accepted_at_usec, critical_command *command)
{
	if (!command || !accepted_at_usec)
		return error::invalid_identity;
	try
	{
		std::vector<uint8_t> witness;
		auto status = economic_baseline_encode(prepared, &witness);
		if (status != error::ok)
			return status;
		economic_digest digest;
		SHA256(witness.data(), witness.size(), digest.data());
		critical_command result = {};
		result.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		result.operation_id = prepared.plan().metadata.operation_id;
		result.type = critical_command_type::economic_baseline;
		result.payload_version = 1;
		result.source_site = critical_source_site::operator_repair;
		result.deadline_class = critical_deadline_class::recovery;
		result.accepted_at_usec = accepted_at_usec;
		result.keys.push_back({ critical_entity_type::system, ECONOMIC_BASELINE_FENCE });
		result.payload.resize(ECONOMIC_BASELINE_COMMAND_BYTES);
		const std::array<uint8_t, 4> magic = { 'E', 'B', 'C', '1' };
		std::copy(magic.begin(), magic.end(), result.payload.begin());
		put(result.payload, 4, 1, 2);
		put(result.payload, 6, ECONOMIC_BASELINE_COMMAND_BYTES, 2);
		put(result.payload, 8, witness.size(), 4);
		std::copy(digest.begin(), digest.end(), result.payload.begin() + 16);
		economic_admission_facts facts;
		facts.metadata = prepared.plan().metadata;
		status = economic_intent_freeze(result, facts, &result.accounting_intent);
		if (status != error::ok)
			return status;
		result.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		if (!critical_command_envelope_valid(result))
			return error::corrupt_evidence;
		*command = std::move(result);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error economic_baseline_command_plan(const critical_command &command,
							 const economic_prepared_baseline &prepared,
							 economic_accounting_plan *plan)
{
	if (!plan)
		return error::invalid_identity;
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::economic_baseline ||
	    !critical_command_envelope_valid(command))
		return error::corrupt_evidence;
	try
	{
		critical_command expected;
		auto status = economic_baseline_command_build(prepared, command.accepted_at_usec,
							      &expected);
		if (status != error::ok)
			return status;
		// Compare every field via canonical wire bytes; preserve allocation
		// failure as capacity rather than misreporting it as a payload conflict.
		std::vector<uint8_t> actual_bytes, expected_bytes;
		if (critical_command_encode(command, &actual_bytes) !=
			    critical_command_codec_result::ok ||
		    critical_command_encode(expected, &expected_bytes) !=
			    critical_command_codec_result::ok)
			return error::capacity;
		if (actual_bytes != expected_bytes)
			return error::payload_conflict;
		economic_frozen_intent intent;
		status = economic_intent_decode(command.accounting_intent, &intent);
		if (status != error::ok)
			return status;
		auto result = prepared.plan();
		status = economic_intent_plan_metadata(command, intent, &result.metadata);
		if (status != error::ok)
			return status;
		// All effects came from the prepared witness. Check the fully bound
		// plan through the common compiler before returning it to a store owner.
		std::vector<uint8_t> encoded;
		status = economic_plan_encode(result, &encoded);
		if (status != error::ok)
			return status;
		*plan = std::move(result);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}
