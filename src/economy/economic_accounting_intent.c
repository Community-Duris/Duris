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
	// Freezing structural evidence is independent of execution support.
	if (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    !command.accounting_intent.empty())
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

namespace
{
[[maybe_unused]] bool intent_bound_add(size_t &bytes, size_t amount) noexcept
{
	if (amount > SIZE_MAX - bytes)
		return false;
	bytes += amount;
	return true;
}
[[maybe_unused]] bool intent_bound_array(size_t &bytes, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && intent_bound_add(bytes, count * width);
}
[[maybe_unused]] bool intent_bound_prepend(size_t old_size, size_t tag_size, size_t &capacity,
					   size_t &peak) noexcept
{
	// Fresh vector(size); pinned libstdc++ insertion retains the old allocation
	// while allocating size + max(size, inserted_count). Tags include their NUL.
	capacity = old_size;
	if (!intent_bound_add(capacity, std::max(old_size, tag_size)))
		return false;
	peak = old_size;
	return intent_bound_add(peak, capacity);
}
struct intent_metadata_bound_workspace
{
	size_t base = 0, peak = 0, phase = 0, bytes = 0, fixed = 0;
	size_t command_heap = 0, command_wire = 0;
	size_t binding_capacity = 0, binding_peak = 0;
	size_t domain_bytes = 0, domain_capacity = 0, domain_peak = 0;
	size_t intent_capacity = 0, intent_peak = 0, digest_live = 0;
};
} // namespace

economic_accounting_error economic_intent_decode_bounded(const std::span<const uint8_t> &encoded,
							 economic_frozen_intent *output,
							 bool (*reserve)(size_t, void *) noexcept,
							 void *context, size_t outer_live) noexcept
{
	using error = economic_accounting_error;
	if (!output || encoded.size() < ECONOMIC_INTENT_HEADER_BYTES)
		return error::corrupt_evidence;
	if (encoded.size() > ECONOMIC_ACCOUNTING_MAX_INTENT_BYTES || !reserve)
		return error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live;
	return error::capacity;
#else
	// The original decoder's result and by-value input span stay live. Source
	// decoding dies before facts.assign, so its DTOs and the suffix request are
	// sequential peaks. Header/valid checks also pass a transient span by value.
	size_t fixed = sizeof(std::span<const uint8_t>);
	if (encoded[27] == 1)
	{
		size_t source = sizeof(economic_source_event);
		if (!intent_bound_add(source, economic_source_event_decode_object_bytes()) ||
		    !intent_bound_add(source, sizeof(std::span<const uint8_t>)))
			return error::capacity;
		fixed = std::max(fixed, source);
	}
	size_t peak = outer_live;
	if (!intent_bound_add(peak, sizeof(economic_frozen_intent)) ||
	    !intent_bound_add(peak, sizeof(std::span<const uint8_t>)) ||
	    !intent_bound_add(peak,
			      std::max(fixed, encoded.size() - ECONOMIC_INTENT_HEADER_BYTES)) ||
	    !reserve(peak, context))
		return error::capacity;
	try
	{
		return economic_intent_decode(encoded, output);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
#endif
}

namespace
{
economic_accounting_error intent_proof_bounded(const critical_command &command,
					       const economic_frozen_intent &intent,
					       economic_plan_metadata *output,
					       bool metadata_requested,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live) noexcept
{
	using error = economic_accounting_error;
	if (metadata_requested && !output)
		return error::corrupt_evidence;
	if (!reserve)
		return error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)intent;
	(void)context;
	(void)outer_live;
	(void)metadata_requested;
	return error::capacity;
#else
	size_t base = outer_live;
	if (!intent_bound_add(base, sizeof(intent_metadata_bound_workspace)) ||
	    !intent_bound_add(base, sizeof(std::span<const uint8_t>)) || !reserve(base, context))
		return error::capacity;
	auto status = valid(intent);
	if (status != error::ok)
		return status;
	if (!critical_operation_id_equal(command.operation_id,
					 intent.admission.metadata.operation_id))
		return error::payload_conflict;
	intent_metadata_bound_workspace work;
	work.base = outer_live;
	if (!intent_bound_add(work.base, sizeof(work)))
		return error::capacity;
	work.peak = base;
	work.bytes = ECONOMIC_INTENT_HEADER_BYTES;
	if (!intent_bound_add(work.bytes, intent.admission.facts.size()))
		return error::capacity;
	work.fixed = intent.admission.metadata.source_event ?
			     2 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>) :
			     sizeof(std::span<uint8_t>);
	// Canonical comparison retains its output vector alongside encode's bytes.
	if (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
	{
		work.phase = work.base;
		if (!intent_bound_add(work.phase, 2 * sizeof(std::vector<uint8_t>)) ||
		    !intent_bound_add(work.phase, work.bytes) ||
		    !intent_bound_add(work.phase, work.fixed))
			return error::capacity;
		work.peak = std::max(work.peak, work.phase);
	}
	// Projection copies every input vector by size; clear() keeps the intent
	// allocation. Its encoder reserves exact schema-1 wire bytes only once.
	work.command_heap = command.payload.size();
	if (!intent_bound_add(work.command_heap, command.accounting_intent.size()) ||
	    !intent_bound_array(work.command_heap, command.keys.size(),
				sizeof(critical_entity_key)) ||
	    !intent_bound_array(work.command_heap, command.expected_revisions.size(),
				sizeof(critical_expected_revision)))
		return error::capacity;
	work.command_wire = CRITICAL_COMMAND_HEADER_BYTES;
	if (!intent_bound_array(work.command_wire, command.keys.size(),
				CRITICAL_COMMAND_ENTITY_KEY_BYTES) ||
	    !intent_bound_array(work.command_wire, command.expected_revisions.size(),
				CRITICAL_COMMAND_EXPECTED_REVISION_BYTES) ||
	    !intent_bound_add(work.command_wire, command.payload.size()) ||
	    !intent_bound_prepend(work.command_wire, sizeof("DURIS-ECONOMIC-COMMAND-V1"),
				  work.binding_capacity, work.binding_peak))
		return error::capacity;
	work.digest_live = work.binding_capacity;
	if (!intent_bound_add(work.digest_live, sizeof(economic_digest)))
		return error::capacity;
	size_t command_encode_peak = work.command_wire;
	if (!intent_bound_add(command_encode_peak, sizeof(std::vector<uint8_t>)))
		return error::capacity;
	work.phase = work.base;
	if (!intent_bound_add(work.phase, sizeof(economic_digest)) ||
	    !intent_bound_add(work.phase, sizeof(critical_command)) ||
	    !intent_bound_add(work.phase, work.command_heap) ||
	    !intent_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !intent_bound_add(work.phase, std::max(command_encode_peak,
						   std::max(work.binding_peak, work.digest_live))))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	// The binding result persists across domain_hash; the projection is gone.
	work.domain_bytes = command.payload.size();
	if (!intent_bound_add(work.domain_bytes, 8) ||
	    !intent_bound_prepend(work.domain_bytes, sizeof("DURIS-ECONOMIC-DOMAIN-V1"),
				  work.domain_capacity, work.domain_peak))
		return error::capacity;
	work.digest_live = work.domain_capacity;
	if (!intent_bound_add(work.digest_live, 2 * sizeof(economic_digest)))
		return error::capacity;
	work.phase = work.base;
	if (!intent_bound_add(work.phase, sizeof(economic_digest)) ||
	    !intent_bound_add(work.phase, 2 * sizeof(std::vector<uint8_t>)) ||
	    !intent_bound_add(work.phase, std::max(work.domain_peak, work.digest_live)))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	if (metadata_requested)
	{
		// Only after binding verification does the original construct metadata and
		// digest the canonical intent. Moved hash parameter and source vector objects
		// coexist; prepend's old/new requests and digest DTOs are separate phases.
		if (!intent_bound_prepend(work.bytes, sizeof("DURIS-ECONOMIC-INTENT-V1"),
					  work.intent_capacity, work.intent_peak))
			return error::capacity;
		work.digest_live = work.intent_capacity;
		if (!intent_bound_add(work.digest_live, 2 * sizeof(economic_digest)))
			return error::capacity;
		work.phase = work.base;
		size_t encode_peak = work.bytes;
		if (!intent_bound_add(encode_peak, work.fixed) ||
		    !intent_bound_add(work.phase, sizeof(economic_plan_metadata)) ||
		    !intent_bound_add(work.phase, 2 * sizeof(std::vector<uint8_t>)) ||
		    !intent_bound_add(work.phase,
				      std::max(encode_peak,
					       std::max(work.intent_peak, work.digest_live))))
			return error::capacity;
		work.peak = std::max(work.peak, work.phase);
	}
	if (!reserve(work.peak, context))
		return error::capacity;
	try
	{
		return metadata_requested ? economic_intent_plan_metadata(command, intent, output) :
					    economic_intent_verify_binding(command, intent);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
#endif
}
} // namespace

economic_accounting_error economic_intent_plan_metadata_bounded(
	const critical_command &command, const economic_frozen_intent &intent,
	economic_plan_metadata *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	return intent_proof_bounded(command, intent, output, true, reserve, context, outer_live);
}
economic_accounting_error economic_intent_verify_binding_bounded(
	const critical_command &command, const economic_frozen_intent &intent,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	// Binding-only verification must not compute the later metadata digest.
	return intent_proof_bounded(command, intent, nullptr, false, reserve, context, outer_live);
}

namespace
{
struct intent_freeze_bound_workspace
{
	size_t base = 0, peak = 0, phase = 0, clone = 0, wire = 0;
	size_t binding_capacity = 0, binding_reallocation = 0;
	size_t domain_size = 0, domain_capacity = 0, domain_reallocation = 0;
	size_t nested = 0, retained = 0, encoded_size = 0;
};
bool intent_codec_admit(size_t base, size_t added, bool (*reserve)(size_t, void *) noexcept,
			void *context) noexcept
{
	return added <= SIZE_MAX - base && reserve && reserve(base + added, context);
}
// This phase excludes caller intent/input/old output. Source arrays survive
// the nested encoder result; mutable write spans belong to later phases.
bool intent_encode_working_bytes(const economic_frozen_intent &intent, size_t &working) noexcept
{
	working = sizeof(std::vector<uint8_t>);
	if (!intent_bound_add(working, ECONOMIC_INTENT_HEADER_BYTES) ||
	    !intent_bound_add(working, intent.admission.facts.size()) ||
	    !intent_bound_add(
		    working,
		    intent.admission.metadata.source_event ?
			    std::max(2 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>),
				     sizeof(std::span<uint8_t>)) :
			    sizeof(std::span<uint8_t>)))
		return false;
	// valid() has a by-value zero() span before encoder bytes construction.
	working = std::max(working, sizeof(std::span<const uint8_t>));
	return true;
}
}

economic_accounting_error economic_intent_encode_bounded(const economic_frozen_intent &intent,
							 std::vector<uint8_t> *encoded,
							 bool (*reserve)(size_t, void *) noexcept,
							 void *context, size_t outer_live) noexcept
{
	using error = economic_accounting_error;
	if (!encoded)
		return error::corrupt_evidence;
	if (!intent_codec_admit(outer_live, sizeof(std::span<const uint8_t>), reserve, context))
		return error::capacity;
	const auto status = valid(intent);
	if (status != error::ok)
		return status;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return error::capacity;
#else
	size_t working = 0;
	if (!intent_encode_working_bytes(intent, working) ||
	    !intent_codec_admit(outer_live, working, reserve, context))
		return error::capacity;
	try
	{
		return economic_intent_encode(intent, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
#endif
}

economic_accounting_error economic_intent_freeze_bounded(const critical_command &command,
							 const economic_admission_facts &facts,
							 std::vector<uint8_t> *encoded,
							 bool (*reserve)(size_t, void *) noexcept,
							 void *context, size_t outer_live) noexcept
{
	using error = economic_accounting_error;
	// Keep the original freeze predicates/order before any prospective profile.
	if (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    !command.accounting_intent.empty())
		return error::invalid_version;
	if (!encoded)
		return error::corrupt_evidence;
	if (facts.facts.size() > ECONOMIC_INTENT_MAX_FACT_BYTES)
		return error::capacity;
	if (!critical_operation_id_is_zero(facts.metadata.operation_id) &&
	    !critical_operation_id_equal(facts.metadata.operation_id, command.operation_id))
		return error::payload_conflict;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)reserve;
	(void)context;
	(void)outer_live;
	return error::capacity;
#else
	const size_t max_keys = critical_command_native_auction_envelope(command) ?
					CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS :
					CRITICAL_COMMAND_MAX_KEYS;
	if (command.keys.size() > max_keys || command.expected_revisions.size() > max_keys ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return error::capacity;
	if (!intent_codec_admit(outer_live, sizeof(intent_freeze_bound_workspace), reserve,
				context))
		return error::capacity;
	intent_freeze_bound_workspace work;
	work.base = outer_live;
	if (!intent_bound_add(work.base, sizeof(work)) ||
	    !intent_bound_add(work.base, sizeof(economic_frozen_intent)))
		return error::capacity;
	// Binding projection copies each vector by size, retaining cleared intent
	// storage if present (the original freeze guard requires that size zero).
	work.clone = command.payload.size();
	if (!intent_bound_add(work.clone, command.accounting_intent.size()) ||
	    !intent_bound_array(work.clone, command.keys.size(), sizeof(critical_entity_key)) ||
	    !intent_bound_array(work.clone, command.expected_revisions.size(),
				sizeof(critical_expected_revision)))
		return error::capacity;
	work.wire = CRITICAL_COMMAND_HEADER_BYTES;
	if (!intent_bound_array(work.wire, command.keys.size(),
				CRITICAL_COMMAND_ENTITY_KEY_BYTES) ||
	    !intent_bound_array(work.wire, command.expected_revisions.size(),
				CRITICAL_COMMAND_EXPECTED_REVISION_BYTES) ||
	    !intent_bound_add(work.wire, command.payload.size()) ||
	    !intent_bound_prepend(work.wire, sizeof("DURIS-ECONOMIC-COMMAND-V1"),
				  work.binding_capacity, work.binding_reallocation))
		return error::capacity;
	// Original encoded output vector remains alongside the encoder's own fresh
	// reserve; its later prepend and result digest are separate actual phases.
	work.nested = sizeof(std::vector<uint8_t>);
	if (!intent_bound_add(work.nested, work.wire))
		return error::capacity;
	work.nested = std::max(work.nested, work.binding_reallocation);
	work.phase = work.binding_capacity;
	if (!intent_bound_add(work.phase, sizeof(economic_digest)))
		return error::capacity;
	work.nested = std::max(work.nested, work.phase);
	work.peak = work.base;
	if (!intent_bound_add(work.peak, sizeof(critical_command)) ||
	    !intent_bound_add(work.peak, work.clone) ||
	    !intent_bound_add(work.peak, sizeof(std::vector<uint8_t>)) ||
	    !intent_bound_add(work.peak, work.nested))
		return error::capacity;
	// Admission facts copy is made only after projection/binding buffers die.
	work.retained = work.base;
	if (!intent_bound_add(work.retained, facts.facts.size()))
		return error::capacity;
	work.peak = std::max(work.peak, work.retained);
	work.domain_size = command.payload.size();
	if (!intent_bound_add(work.domain_size, 8) ||
	    !intent_bound_prepend(work.domain_size, sizeof("DURIS-ECONOMIC-DOMAIN-V1"),
				  work.domain_capacity, work.domain_reallocation))
		return error::capacity;
	// Domain put(span) precedes moved hash parameter construction. Afterwards
	// both vector objects survive old/new prepend requests and returned digest.
	work.phase = work.retained;
	if (!intent_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !intent_bound_add(work.phase, work.domain_size) ||
	    !intent_bound_add(work.phase, sizeof(std::span<uint8_t>)))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	work.nested = work.domain_capacity;
	if (!intent_bound_add(work.nested, 2 * sizeof(economic_digest)))
		return error::capacity;
	work.nested = std::max(work.nested, work.domain_reallocation);
	work.phase = work.retained;
	if (!intent_bound_add(work.phase, 2 * sizeof(std::vector<uint8_t>)) ||
	    !intent_bound_add(work.phase, work.nested))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	// Final intent validation span precedes fresh exact header+facts vector.
	work.encoded_size = ECONOMIC_INTENT_HEADER_BYTES;
	if (!intent_bound_add(work.encoded_size, facts.facts.size()))
		return error::capacity;
	work.nested = sizeof(std::vector<uint8_t>);
	if (!intent_bound_add(work.nested, work.encoded_size) ||
	    !intent_bound_add(
		    work.nested,
		    facts.metadata.source_event ?
			    std::max(2 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>),
				     sizeof(std::span<uint8_t>)) :
			    sizeof(std::span<uint8_t>)))
		return error::capacity;
	work.nested = std::max(work.nested, sizeof(std::span<const uint8_t>));
	work.phase = work.retained;
	if (!intent_bound_add(work.phase, work.nested))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	if (!reserve(work.peak, context))
		return error::capacity;
	try
	{
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
#endif
}

namespace
{
bool intent_fixed_add(size_t &total, size_t extra) noexcept
{
	if (extra > SIZE_MAX - total)
		return false;
	total += extra;
	return true;
}
constexpr size_t intent_fixed_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t intent_fixed_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t intent_fixed_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t intent_fixed_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t intent_fixed_vector_frames =
	intent_fixed_allocator_frames + intent_fixed_copy_frames + intent_fixed_relocate_frames +
	intent_fixed_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t intent_fixed_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + intent_fixed_allocator_frames;
constexpr size_t intent_fixed_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + intent_fixed_vector_frames;
constexpr size_t intent_fixed_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t intent_fixed_sha_c_small_frames =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(const uint8_t *);
constexpr size_t intent_fixed_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						    11 * sizeof(unsigned int) + 2 * sizeof(int) +
						    2 * sizeof(void *);
constexpr size_t intent_fixed_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t intent_fixed_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
						  2 * sizeof(void *) + sizeof(unsigned int) +
						  sizeof(size_t) + sizeof(int);
constexpr size_t intent_fixed_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
						 sizeof(unsigned long) + sizeof(unsigned int) +
						 sizeof(int);
[[maybe_unused]] constexpr size_t intent_fixed_sha_frames =
	std::max(intent_fixed_sha_assembly_frames,
		 std::max(intent_fixed_sha_c_small_frames, intent_fixed_sha_c_normal_frames)) +
	std::max(intent_fixed_sha_init_frames,
		 std::max(intent_fixed_sha_update_frames, intent_fixed_sha_final_frames));

constexpr size_t intent_fixed_count_constructor_frames =
	// Actual vector(n,a) and _S_check_init_len/_Vector_base(n,a), copied
	// allocator/impl/data/_M_create_storage and default initialize(n).
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint8_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t) + sizeof(void *) + sizeof(size_t) + intent_fixed_vector_frames;
constexpr size_t intent_fixed_intent_default_frames =
	// Actual frozen/admission/metadata aggregate default/destructor this
	// scopes, fixed optional source event, one facts vector and real base/
	// impl/data default path. Inline actual intent is separately owned.
	6 * sizeof(void *) + 4 * sizeof(void *) + sizeof(std::allocator<uint8_t>) +
	4 * sizeof(void *) + sizeof(bool) + sizeof(void *) + intent_fixed_allocator_frames;
constexpr size_t intent_fixed_encode_frames =
	// Original encode + bounded encode parameter/return/meta/status/working
	// and intent_codec_admit scalars/parameters, encode byte-vector count
	// constructor plus actual final move/destructor. Source/event arrays
	// and encoded vector requests belong to existing owning encode helper.
	10 * sizeof(void *) + 5 * sizeof(size_t) + 4 * sizeof(economic_accounting_error) +
	intent_fixed_count_constructor_frames + intent_fixed_move_frames +
	// valid/zero(span)/metadata validate/rule_for/source-kind/source-event
	// valid and zero-ID/pair-ID argument and return carriers; actual optional
	// query/get/has_value. Rule lookup is fixed range no allocation.
	4 * sizeof(void *) + 2 * sizeof(economic_accounting_error) +
	2 * sizeof(std::span<const uint8_t>) + sizeof(bool) + 8 * sizeof(void *) +
	sizeof(economic_reason) + sizeof(economic_source_kind) + 3 * sizeof(bool) +
	4 * (2 * sizeof(void *) + sizeof(bool)) + sizeof(uint8_t) +
	6 * (sizeof(void *) + sizeof(bool)) +
	// zero/all_of/find_if_not/find_if/__find_if ordinary RA declarations,
	// predicate functor/RA tag/trip count and real lambda this/value/result.
	5 * (3 * sizeof(void *) + sizeof(char) + sizeof(bool)) +
	sizeof(std::random_access_iterator_tag) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(uint8_t) + sizeof(bool) +
	// Original put(span,offset,value,length,index) and write_array(actual
	// span by value,offset,arrayref) and pointer-copy full source profile.
	2 * sizeof(std::span<uint8_t>) + 4 * sizeof(size_t) + sizeof(uint64_t) +
	2 * sizeof(void *) + intent_fixed_copy_frames + 8 * (sizeof(void *) + sizeof(size_t)) +
	// Genuine original source_event_encode event/output/kind/byte/result;
	// internal fixed result array is already owned by existing encode helper.
	2 * sizeof(void *) + sizeof(uint16_t) + 2 * sizeof(size_t) +
	sizeof(economic_accounting_error);
struct intent_fixed_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const economic_frozen_intent *intent = nullptr;
	const std::vector<uint8_t> *domain_bytes = nullptr, *hash_bytes = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 11 * sizeof(void *) + 7 * sizeof(size_t) +
					       6 * sizeof(bool) +
					       6 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer;
		if (!intent_fixed_add(total, sizeof(*this)) || !intent_fixed_add(total, frames) ||
		    !intent_fixed_add(total, observation))
			return false;
		if (intent && (!intent_fixed_add(total, sizeof(*intent)) ||
			       !intent_fixed_add(total, intent->admission.facts.capacity())))
			return false;
		if (domain_bytes && (!intent_fixed_add(total, sizeof(*domain_bytes)) ||
				     !intent_fixed_add(total, domain_bytes->capacity())))
			return false;
		if (hash_bytes && (!intent_fixed_add(total, sizeof(*hash_bytes)) ||
				   !intent_fixed_add(total, hash_bytes->capacity())))
			return false;
		if (!intent_fixed_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	bool growth(const std::vector<uint8_t> &value, size_t count) const noexcept
	{
		size_t request = intent_fixed_vector_frames;
		if (count > value.max_size() - value.size())
			return false;
		if (count > value.capacity() - value.size())
		{
			size_t next = value.size();
			if (!intent_fixed_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (!intent_fixed_add(request, next))
				return false;
		}
		return intent_fixed_add(request,
					2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
	bool assign_facts(const std::vector<uint8_t> &destination, size_t count) const noexcept
	{
		size_t request = intent_fixed_vector_frames +
				 // Admission/metadata generated COPY assignment, source-event
				 // fixed optional assignment and real vector operator=(const&).
				 4 * sizeof(void *) + 8 * sizeof(void *) + 2 * sizeof(bool) +
				 2 * sizeof(void *);
		if (count > destination.max_size() ||
		    (count > destination.capacity() && !intent_fixed_add(request, count)))
			return false;
		return intent_fixed_add(request,
					2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
};
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
template <size_t N> economic_accounting_error intent_fixed_hash_owned(const char (&tag)[N],
								      std::vector<uint8_t> bytes,
								      economic_digest *digest,
								      intent_fixed_budget &budget)
{
	budget.hash_bytes = &bytes;
	if (!budget.growth(bytes, N))
		return economic_accounting_error::capacity;
	bytes.insert(bytes.begin(), tag, tag + N);
	if (!budget.peak(sizeof(economic_digest) + sizeof(SHA256_CTX) + intent_fixed_sha_frames +
			 3 * (sizeof(void *) + sizeof(size_t))))
		return economic_accounting_error::capacity;
	economic_digest result = {};
	SHA256_CTX digest_context;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	if (SHA256_Init(&digest_context) != 1 ||
	    SHA256_Update(&digest_context, bytes.data(), bytes.size()) != 1 ||
	    SHA256_Final(result.data(), &digest_context) != 1)
		return economic_accounting_error::corrupt_evidence;
#pragma GCC diagnostic pop
	*digest = result;
	budget.hash_bytes = nullptr;
	return economic_accounting_error::ok;
}

economic_accounting_error intent_fixed_domain_hash_owned(const critical_command &command,
							 economic_digest *digest,
							 intent_fixed_budget &budget)
{
	size_t admission_request = 8;
	if (!intent_fixed_add(admission_request, command.payload.size()) ||
	    !intent_fixed_add(admission_request, sizeof(std::vector<uint8_t>) +
							 intent_fixed_count_constructor_frames) ||
	    !budget.peak(admission_request))
		return economic_accounting_error::capacity;
	std::vector<uint8_t> bytes(8 + command.payload.size());
	budget.domain_bytes = &bytes;
	put(bytes, 0, static_cast<uint16_t>(command.type), 2);
	put(bytes, 2, command.payload_version, 2);
	put(bytes, 4, command.payload.size(), 4);
	std::copy(command.payload.begin(), command.payload.end(), bytes.begin() + 8);
	if (!budget.peak(sizeof(std::vector<uint8_t>) + intent_fixed_move_frames))
		return economic_accounting_error::capacity;
	const auto admission_status = intent_fixed_hash_owned("DURIS-ECONOMIC-DOMAIN-V1",
							      std::move(bytes), digest, budget);
	budget.domain_bytes = nullptr;
	return admission_status;
}

economic_accounting_error intent_fixed_freeze_owned(const critical_command &command,
						    const economic_admission_facts &facts,
						    std::vector<uint8_t> *encoded,
						    intent_fixed_budget &budget)
{
	// Freezing structural evidence is independent of execution support.
	if (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    !command.accounting_intent.empty())
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
		size_t admission_prefix = 0, admission_request = 0;
		if (!budget.peak(sizeof(economic_frozen_intent) +
				 intent_fixed_intent_default_frames))
			return economic_accounting_error::capacity;
		economic_frozen_intent intent;
		budget.intent = &intent;
		auto status = (!budget.prefix(admission_prefix) ?
				       economic_accounting_error::capacity :
				       economic_command_binding_digest_bounded(
					       command, &intent.command_binding, budget.reserve,
					       budget.context, admission_prefix));
		if (status != economic_accounting_error::ok)
			return status;
		if (!budget.assign_facts(intent.admission.facts, facts.facts.size()))
			return economic_accounting_error::capacity;
		intent.admission = facts;
		intent.admission.metadata.operation_id = command.operation_id;
		status = intent_fixed_domain_hash_owned(command, &intent.domain_digest, budget);
		if (status != economic_accounting_error::ok)
			return status;
		return !budget.prefix(admission_prefix, intent_fixed_encode_frames) ?
			       economic_accounting_error::capacity :
			       economic_intent_encode_bounded(intent, encoded, budget.reserve,
							      budget.context, admission_prefix);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
}
#endif
} // namespace
economic_accounting_error economic_intent_freeze_fixed_bounded(
	const critical_command &command, const economic_admission_facts &facts,
	std::vector<uint8_t> *encoded, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	if (!reserve)
		return economic_accounting_error::capacity;
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)

	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return economic_accounting_error::capacity;
	constexpr size_t frames =
		// Public/owned freeze params/status/prefix/request/catch-reference;
		// domain/hash owned references/digest/error and parameter vector
		// carriers are genuine owned by observers rather than counted twice.
		9 * sizeof(void *) + sizeof(size_t) + 3 * sizeof(economic_accounting_error) +
		2 * sizeof(size_t) + sizeof(void *) + 5 * sizeof(void *) +
		sizeof(economic_accounting_error) + sizeof(size_t) +
		sizeof(economic_accounting_error) +
		// Original put/span and payload std::copy profile through domain fill.
		sizeof(std::span<uint8_t>) + 3 * sizeof(size_t) + sizeof(uint64_t) +
		intent_fixed_copy_frames + 8 * (sizeof(void *) + sizeof(size_t));
	intent_fixed_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(critical_command_valid_frame_bytes()))
		return economic_accounting_error::capacity;
	return intent_fixed_freeze_owned(command, facts, encoded, budget);
#else
	(void)command;
	(void)facts;
	(void)encoded;
	(void)context;
	(void)outer_live;
	return economic_accounting_error::capacity;
#endif
}
