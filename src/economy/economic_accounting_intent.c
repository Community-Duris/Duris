#include "economy/economic_accounting_intent.h"

#include <algorithm>
#include <new>
#include <openssl/sha.h>
#include <utility>

namespace
{
static_assert(ECONOMIC_ACCOUNTING_MAX_INTENT_BYTES == CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES);
constexpr std::array<uint8_t, 4> MAGIC = { 'E', 'A', 'I', '1' };
void put(std::span<uint8_t> bytes, size_t offset, uint64_t value, size_t length)
{
	for (size_t index = 0; index < length; ++index)
		bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8));
}
uint64_t get(std::span<const uint8_t> bytes, size_t offset, size_t length)
{
	uint64_t result = 0;
	for (size_t index = 0; index < length; ++index)
		result |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
	return result;
}
bool zero(std::span<const uint8_t> bytes)
{
	return std::all_of(bytes.begin(), bytes.end(), [](uint8_t value) { return value == 0; });
}
template <size_t N>
void write_array(std::span<uint8_t> bytes, size_t offset, const std::array<uint8_t, N> &value)
{
	std::copy(value.begin(), value.end(), bytes.begin() + offset);
}
template <size_t N>
void read_array(std::span<const uint8_t> bytes, size_t offset, std::array<uint8_t, N> &value)
{
	std::copy_n(bytes.begin() + offset, N, value.begin());
}
economic_accounting_error valid(const economic_frozen_intent &intent)
{
	if (intent.admission.facts.size() > ECONOMIC_INTENT_MAX_FACT_BYTES)
		return economic_accounting_error::capacity;
	if (intent.admission.facts_version != 1)
		return economic_accounting_error::invalid_version;
	if (zero(intent.command_binding) || zero(intent.domain_digest))
		return economic_accounting_error::invalid_identity;
	return economic_operation_metadata_validate(intent.admission.metadata);
}
// Callers check bounds before reaching this allocation. Hash tags include NUL.
template <size_t N> economic_digest hash(const char (&tag)[N], std::vector<uint8_t> bytes)
{
	bytes.insert(bytes.begin(), tag, tag + N);
	economic_digest result = {};
	SHA256(bytes.data(), bytes.size(), result.data());
	return result;
}
economic_digest domain_hash(const critical_command &command)
{
	std::vector<uint8_t> bytes(8 + command.payload.size());
	put(bytes, 0, static_cast<uint16_t>(command.type), 2);
	put(bytes, 2, command.payload_version, 2);
	put(bytes, 4, command.payload.size(), 4);
	std::copy(command.payload.begin(), command.payload.end(), bytes.begin() + 8);
	return hash("DURIS-ECONOMIC-DOMAIN-V1", std::move(bytes));
}
}

economic_accounting_error economic_intent_encode(const economic_frozen_intent &intent,
						 std::vector<uint8_t> *encoded)
{
	if (!encoded)
		return economic_accounting_error::corrupt_evidence;
	auto status = valid(intent);
	if (status != economic_accounting_error::ok)
		return status;
	try
	{
		const auto &meta = intent.admission.metadata;
		std::vector<uint8_t> bytes(ECONOMIC_INTENT_HEADER_BYTES +
					   intent.admission.facts.size());
		write_array(bytes, 0, MAGIC);
		put(bytes, 4, meta.version, 2);
		put(bytes, 6, ECONOMIC_INTENT_HEADER_BYTES, 2);
		put(bytes, 8, bytes.size(), 4);
		put(bytes, 12, meta.writer_id, 4);
		put(bytes, 16, meta.policy_version, 4);
		put(bytes, 20, meta.compiler_version, 4);
		put(bytes, 24, static_cast<uint16_t>(meta.reason), 2);
		bytes[26] = static_cast<uint8_t>(meta.actor_kind);
		bytes[27] = meta.source_event ? 1 : 0;
		put(bytes, 28, intent.admission.facts_version, 2);
		write_array(bytes, 32, meta.lineage.bytes);
		write_array(bytes, 48, meta.epoch.bytes);
		write_array(bytes, 64, meta.operation_id.bytes);
		write_array(bytes, 80, meta.original_operation_id.bytes);
		put(bytes, 96, meta.actor_id, 8);
		put(bytes, 104, intent.admission.facts.size(), 4);
		if (meta.source_event)
		{
			std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source = {};
			status = economic_source_event_encode(*meta.source_event, &source);
			if (status != economic_accounting_error::ok)
				return status;
			write_array(bytes, 112, source);
		}
		write_array(bytes, 160, intent.command_binding);
		write_array(bytes, 192, intent.domain_digest);
		std::copy(intent.admission.facts.begin(), intent.admission.facts.end(),
			  bytes.begin() + 256);
		*encoded = std::move(bytes);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error economic_intent_decode(std::span<const uint8_t> encoded,
						 economic_frozen_intent *intent)
{
	if (!intent || encoded.size() < ECONOMIC_INTENT_HEADER_BYTES)
		return economic_accounting_error::corrupt_evidence;
	if (encoded.size() > ECONOMIC_ACCOUNTING_MAX_INTENT_BYTES)
		return economic_accounting_error::capacity;
	if (!std::equal(MAGIC.begin(), MAGIC.end(), encoded.begin()) ||
	    get(encoded, 6, 2) != ECONOMIC_INTENT_HEADER_BYTES ||
	    get(encoded, 8, 4) != encoded.size() ||
	    get(encoded, 104, 4) != encoded.size() - ECONOMIC_INTENT_HEADER_BYTES ||
	    encoded[27] > 1 || !zero(encoded.subspan(30, 2)) || !zero(encoded.subspan(108, 4)) ||
	    !zero(encoded.subspan(224, 32)))
		return economic_accounting_error::corrupt_evidence;
	try
	{
		economic_frozen_intent result;
		auto &meta = result.admission.metadata;
		meta.version = static_cast<uint16_t>(get(encoded, 4, 2));
		meta.writer_id = static_cast<uint32_t>(get(encoded, 12, 4));
		meta.policy_version = static_cast<uint32_t>(get(encoded, 16, 4));
		meta.compiler_version = static_cast<uint32_t>(get(encoded, 20, 4));
		meta.reason = static_cast<economic_reason>(get(encoded, 24, 2));
		meta.actor_kind = static_cast<economic_actor_kind>(encoded[26]);
		result.admission.facts_version = static_cast<uint16_t>(get(encoded, 28, 2));
		read_array(encoded, 32, meta.lineage.bytes);
		read_array(encoded, 48, meta.epoch.bytes);
		read_array(encoded, 64, meta.operation_id.bytes);
		read_array(encoded, 80, meta.original_operation_id.bytes);
		meta.actor_id = get(encoded, 96, 8);
		if (encoded[27])
		{
			economic_source_event source;
			const auto status =
				economic_source_event_decode(encoded.subspan(112, 48), &source);
			if (status != economic_accounting_error::ok)
				return status;
			meta.source_event = source;
		}
		else if (!zero(encoded.subspan(112, 48)))
			return economic_accounting_error::corrupt_evidence;
		read_array(encoded, 160, result.command_binding);
		read_array(encoded, 192, result.domain_digest);
		auto status = valid(result);
		if (status != economic_accounting_error::ok)
			return status;
		result.admission.facts.assign(encoded.begin() + 256, encoded.end());
		*intent = std::move(result);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error economic_intent_freeze(const critical_command &command,
						 const economic_admission_facts &facts,
						 std::vector<uint8_t> *encoded)
{
	if (!critical_command_legacy_execution_supported(command))
		return economic_accounting_error::invalid_version;
	if (!encoded)
		return economic_accounting_error::corrupt_evidence;
	if (facts.facts.size() > ECONOMIC_INTENT_MAX_FACT_BYTES)
		return economic_accounting_error::capacity;
	if (!critical_operation_id_is_zero(facts.metadata.operation_id) &&
	    !critical_operation_id_equal(facts.metadata.operation_id, command.operation_id))
		return economic_accounting_error::payload_conflict;
	try
	{
		economic_frozen_intent intent;
		auto status = economic_command_binding_digest(command, &intent.command_binding);
		if (status != economic_accounting_error::ok)
			return status;
		intent.admission = facts;
		intent.admission.metadata.operation_id = command.operation_id;
		intent.domain_digest = domain_hash(command);
		return economic_intent_encode(intent, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
}

economic_accounting_error economic_intent_verify_binding(const critical_command &command,
							 const economic_frozen_intent &intent)
{
	auto status = valid(intent);
	if (status != economic_accounting_error::ok)
		return status;
	if (!critical_operation_id_equal(command.operation_id,
					 intent.admission.metadata.operation_id))
		return economic_accounting_error::payload_conflict;
	try
	{
		if (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		{
			std::vector<uint8_t> canonical;
			status = economic_intent_encode(intent, &canonical);
			if (status != economic_accounting_error::ok)
				return status;
			if (canonical != command.accounting_intent)
				return economic_accounting_error::payload_conflict;
		}
		economic_digest binding = {};
		status = economic_command_binding_digest(command, &binding);
		if (status != economic_accounting_error::ok)
			return status;
		if (binding != intent.command_binding ||
		    domain_hash(command) != intent.domain_digest)
			return economic_accounting_error::payload_conflict;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error economic_intent_digest(const economic_frozen_intent &intent,
						 economic_digest *digest)
{
	if (!digest)
		return economic_accounting_error::corrupt_evidence;
	std::vector<uint8_t> encoded;
	const auto status = economic_intent_encode(intent, &encoded);
	if (status != economic_accounting_error::ok)
		return status;
	try
	{
		*digest = hash("DURIS-ECONOMIC-INTENT-V1", std::move(encoded));
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
	return economic_accounting_error::ok;
}

economic_accounting_error economic_intent_plan_metadata(const critical_command &command,
							const economic_frozen_intent &intent,
							economic_plan_metadata *metadata)
{
	if (!metadata)
		return economic_accounting_error::corrupt_evidence;
	auto status = economic_intent_verify_binding(command, intent);
	if (status != economic_accounting_error::ok)
		return status;
	economic_plan_metadata result;
	static_cast<economic_operation_metadata &>(result) = intent.admission.metadata;
	result.domain_digest = intent.domain_digest;
	status = economic_intent_digest(intent, &result.intent_digest);
	if (status != economic_accounting_error::ok)
		return status;
	*metadata = result;
	return economic_accounting_error::ok;
}
