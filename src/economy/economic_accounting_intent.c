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
