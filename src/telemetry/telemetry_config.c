#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_config_private.h"

#include <climits>
#include <cmath>
#include <cstring>
#include <limits>

#include <openssl/sha.h>

namespace
{

constexpr std::uint32_t FLOAT_0015 = 0x3FC00000U; // 1.5f
constexpr std::uint32_t FLOAT_0020 = 0x40000000U; // 2.0f
constexpr std::uint32_t FLOAT_0100 = 0x3DCCCCCDU; // 0.1f
constexpr std::uint32_t FLOAT_0200 = 0x3E4CCCCDU; // 0.2f
constexpr std::uint32_t FLOAT_0400 = 0x3ECCCCCDU; // 0.4f
constexpr std::uint32_t FLOAT_0002 = 0x3B03126FU; // 0.002f
constexpr std::uint32_t FLOAT_0050 = 0x3D4CCCCDU; // 0.05f

/*
 * This is deliberately a small reviewed allowlist.  The hard-coded entries
 * are the effective branches in limits.c, trophy.c and epic.c.  The rested
 * multipliers remain fixed constants; the separate rested.enabled entry
 * captures automatic applicability (staff grants can override it). Frequency
 * modifiers stay visible
 * as maintained-but-unused evidence, but are excluded from property_version
 * because the payout path does not read them.
 *
 * Registry IDs are append-only.  Keep existing IDs and their catalog mappings
 * intact so sealed historical rows remain resolvable when new context is added.
 */
constexpr telemetry_config_property_definition PROPERTY_REGISTRY[] = {
	{ 1U, telemetry_config_property_role::hardcoded_effective,
	  telemetry_config_property_kind::float32, "rested.xp_multiplier", nullptr, FLOAT_0015 },
	{ 2U, telemetry_config_property_role::hardcoded_effective,
	  telemetry_config_property_kind::float32, "wellrested.xp_multiplier", nullptr,
	  FLOAT_0020 },
	{ 3U, telemetry_config_property_role::hardcoded_effective,
	  telemetry_config_property_kind::boolean, "rested.resurrect_exempt", nullptr, 1U },
	{ 4U, telemetry_config_property_role::effective, telemetry_config_property_kind::boolean,
	  "trophy.exp.zoneTrophy.observe", "exp.zoneTrophy.observe", 0U },
	{ 5U, telemetry_config_property_role::hardcoded_effective,
	  telemetry_config_property_kind::uint32, "trophy.min_level", nullptr, 25U },
	{ 6U, telemetry_config_property_role::hardcoded_effective,
	  telemetry_config_property_kind::uint32, "trophy.max_level", nullptr, 56U },
	{ 7U, telemetry_config_property_role::hardcoded_effective,
	  telemetry_config_property_kind::boolean, "trophy.excludes_illithid", nullptr, 1U },
	{ 8U, telemetry_config_property_role::hardcoded_effective,
	  telemetry_config_property_kind::boolean, "trophy.excludes_pc_pets", nullptr, 1U },
	{ 9U, telemetry_config_property_role::hardcoded_effective,
	  telemetry_config_property_kind::boolean, "trophy.xp_penalty", nullptr, 0U },
	{ 10U, telemetry_config_property_role::effective, telemetry_config_property_kind::float32,
	  "payout.epic.touch.maxPayoutFactor", "epic.touch.maxPayoutFactor", 0x41200000U },
	{ 11U, telemetry_config_property_role::effective, telemetry_config_property_kind::float32,
	  "payout.epic.touch.PayoutFactor", "epic.touch.PayoutFactor", 0x3F800000U },
	{ 12U, telemetry_config_property_role::effective, telemetry_config_property_kind::float32,
	  "payout.epic.zone.alignmentMod", "epic.zone.alignmentMod", FLOAT_0100 },
	{ 13U, telemetry_config_property_role::effective, telemetry_config_property_kind::float32,
	  "payout.epic.alignment.minPercentage", "epic.alignment.minPercentage", FLOAT_0100 },
	{ 14U, telemetry_config_property_role::unavailable_context,
	  telemetry_config_property_kind::unknown, "payout.zone_alignment_context", nullptr, 0U },
	{ 15U, telemetry_config_property_role::maintained_but_unused,
	  telemetry_config_property_kind::uint32, "unused.epic.freqMod.tick.waitSecs",
	  "epic.freqMod.tick.waitSecs", 3600U },
	{ 16U, telemetry_config_property_role::maintained_but_unused,
	  telemetry_config_property_kind::float32, "unused.epic.freqMod.tick.add",
	  "epic.freqMod.tick.add", FLOAT_0002 },
	{ 17U, telemetry_config_property_role::maintained_but_unused,
	  telemetry_config_property_kind::float32, "unused.epic.freqMod.touch.sub",
	  "epic.freqMod.touch.sub", FLOAT_0100 },
	{ 18U, telemetry_config_property_role::maintained_but_unused,
	  telemetry_config_property_kind::float32, "unused.epic.freqMod.min", "epic.freqMod.min",
	  FLOAT_0400 },
	{ 19U, telemetry_config_property_role::maintained_but_unused,
	  telemetry_config_property_kind::float32, "unused.epic.freqMod.max", "epic.freqMod.max",
	  FLOAT_0020 },
	{ 20U, telemetry_config_property_role::effective, telemetry_config_property_kind::boolean,
	  "rested.enabled", "exp.rested.enabled", 1U },
};

constexpr std::size_t PROPERTY_REGISTRY_COUNT =
	sizeof(PROPERTY_REGISTRY) / sizeof(PROPERTY_REGISTRY[0]);
constexpr std::size_t PROPERTY_DIGEST_CANONICAL_BYTES =
	2U + 2U + TELEMETRY_CONFIG_PROPERTY_MAX * 12U;

static_assert(PROPERTY_REGISTRY_COUNT == TELEMETRY_CONFIG_PROPERTY_MAX);

telemetry_config_state GLOBAL_STATE{};

bool bytes_equal(const std::uint8_t *left, const std::uint8_t *right, std::size_t size) noexcept
{
	if (left == nullptr || right == nullptr)
		return false;
	return std::memcmp(left, right, size) == 0;
}

void append_be(std::uint8_t *bytes, std::size_t &offset, std::uint64_t value,
	       unsigned int width) noexcept
{
	for (unsigned int index = width; index != 0U; --index)
		bytes[offset++] = static_cast<std::uint8_t>((value >> ((index - 1U) * 8U)) & 0xFFU);
}

void saturating_increment(std::uint64_t &value) noexcept
{
	if (value != std::numeric_limits<std::uint64_t>::max())
		++value;
}

std::uint32_t read_be32(const std::uint8_t *bytes) noexcept
{
	return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
	       (static_cast<std::uint32_t>(bytes[1]) << 16U) |
	       (static_cast<std::uint32_t>(bytes[2]) << 8U) | static_cast<std::uint32_t>(bytes[3]);
}

std::uint64_t read_be64(const std::uint8_t *bytes) noexcept
{
	std::uint64_t value = 0U;
	for (unsigned int index = 0U; index < 8U; ++index)
		value = (value << 8U) | bytes[index];
	return value;
}

std::uint32_t float_bits(float value) noexcept
{
	std::uint32_t bits = 0U;
	static_assert(sizeof(bits) == sizeof(value));
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

bool float_to_typed_bits(const telemetry_config_property_definition &definition, float value,
			 std::uint32_t &bits) noexcept
{
	if (!std::isfinite(value))
		return false;
	switch (definition.kind)
	{
	case telemetry_config_property_kind::float32:
		bits = float_bits(value == 0.0F ? 0.0F : value);
		return true;
	case telemetry_config_property_kind::boolean:
		if (static_cast<double>(value) < INT_MIN || static_cast<double>(value) > INT_MAX)
			return false;
		bits = static_cast<std::uint32_t>(static_cast<int>(value) == 1 ? 1U : 0U);
		return true;
	case telemetry_config_property_kind::uint32:
		if (value < 0.0f || static_cast<double>(value) > UINT_MAX)
			return false;
		bits = static_cast<std::uint32_t>(static_cast<unsigned int>(value));
		return true;
	case telemetry_config_property_kind::unknown:
		break;
	}
	return false;
}

bool property_capture_reserved_is_zero(const telemetry_config_property_capture &capture) noexcept
{
	for (const std::uint8_t byte : capture.reserved)
		if (byte != 0U)
			return false;
	return true;
}

bool property_entry_is_effective(const telemetry_config_property_entry &entry) noexcept
{
	return entry.role !=
	       static_cast<std::uint8_t>(telemetry_config_property_role::maintained_but_unused);
}

void property_digest_for(const telemetry_config_property_snapshot &snapshot,
			 std::uint8_t digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES]) noexcept
{
	/* The registry version has a small, explicit typed encoding of its own. */
	std::uint8_t canonical[2U + 2U + TELEMETRY_CONFIG_PROPERTY_MAX * 8U]{};
	std::size_t offset = 0U;
	append_be(canonical, offset, snapshot.schema_version, 2U);
	std::uint16_t effective_count = 0U;
	for (std::size_t index = 0U; index < snapshot.count; ++index)
		if (property_entry_is_effective(snapshot.entries[index]))
			++effective_count;
	append_be(canonical, offset, effective_count, 2U);
	for (std::size_t index = 0U; index < snapshot.count; ++index)
	{
		const telemetry_config_property_entry &entry = snapshot.entries[index];
		if (!property_entry_is_effective(entry))
			continue;
		append_be(canonical, offset, entry.id, 2U);
		append_be(canonical, offset, entry.role, 1U);
		append_be(canonical, offset, entry.kind, 1U);
		append_be(canonical, offset, entry.value, 4U);
	}
	SHA256(canonical, offset, digest);
}

std::uint32_t property_version_for(const telemetry_config_property_snapshot &snapshot) noexcept
{
	std::uint8_t digest[SHA256_DIGEST_LENGTH]{};
	property_digest_for(snapshot, digest);
	const std::uint32_t version = read_be32(digest);
	return version == 0U ? 1U : version;
}

bool property_snapshot_is_valid(const telemetry_config_property_snapshot &snapshot) noexcept
{
	if (snapshot.schema_version != TELEMETRY_CONFIG_PROPERTY_SCHEMA_VERSION ||
	    snapshot.reserved != 0U || snapshot.count != PROPERTY_REGISTRY_COUNT ||
	    snapshot.property_version == 0U)
		return false;
	for (std::size_t index = 0U; index < snapshot.count; ++index)
	{
		const telemetry_config_property_entry &entry = snapshot.entries[index];
		if (entry.id != PROPERTY_REGISTRY[index].id ||
		    entry.role != static_cast<std::uint8_t>(PROPERTY_REGISTRY[index].role) ||
		    entry.kind != static_cast<std::uint8_t>(PROPERTY_REGISTRY[index].kind) ||
		    entry.reserved[0] != 0U || entry.reserved[1] != 0U || entry.reserved[2] != 0U)
			return false;
	}
	if (snapshot.effective_digest[0] == 0U && snapshot.effective_digest[1] == 0U &&
	    snapshot.effective_digest[2] == 0U && snapshot.effective_digest[3] == 0U)
		return false;
	std::uint8_t digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES]{};
	property_digest_for(snapshot, digest);
	return bytes_equal(digest, snapshot.effective_digest, sizeof(digest));
}

bool property_snapshot_catalog_is_valid(const telemetry_config_property_snapshot &snapshot) noexcept
{
	return property_snapshot_is_valid(snapshot) && snapshot.stable_namespace != 0U &&
	       snapshot.stable_version != 0U;
}

bool snapshot_fingerprint_matches(const telemetry_config_snapshot &snapshot) noexcept
{
	std::uint8_t fingerprint[TELEMETRY_CONFIG_FINGERPRINT_BYTES]{};
	if (!telemetry_config_compute_fingerprint(snapshot, fingerprint, sizeof(fingerprint)))
		return false;
	return bytes_equal(fingerprint, snapshot.fingerprint, sizeof(fingerprint));
}

bool snapshot_identity_equal(const telemetry_config_snapshot &left,
			     const telemetry_config_snapshot &right) noexcept
{
	return left.config_id == right.config_id &&
	       bytes_equal(left.fingerprint, right.fingerprint, TELEMETRY_CONFIG_FINGERPRINT_BYTES);
}

telemetry_config_state_outcome remember_identity(telemetry_config_state &state,
						 const telemetry_config_snapshot &snapshot) noexcept
{
	std::size_t free_index = TELEMETRY_CONFIG_IDENTITY_REGISTRY_MAX;
	for (std::size_t index = 0U; index < TELEMETRY_CONFIG_IDENTITY_REGISTRY_MAX; ++index)
	{
		const telemetry_config_identity_entry &entry = state.identities[index];
		if (entry.used == 0U)
		{
			if (free_index == TELEMETRY_CONFIG_IDENTITY_REGISTRY_MAX)
				free_index = index;
			continue;
		}
		if (entry.config_id == snapshot.config_id ||
		    bytes_equal(entry.fingerprint, snapshot.fingerprint,
				TELEMETRY_CONFIG_FINGERPRINT_BYTES))
			return entry.config_id == snapshot.config_id &&
					       bytes_equal(entry.fingerprint, snapshot.fingerprint,
							   TELEMETRY_CONFIG_FINGERPRINT_BYTES) ?
				       telemetry_config_state_outcome::accepted :
				       telemetry_config_state_outcome::identity_collision;
	}
	if (free_index == TELEMETRY_CONFIG_IDENTITY_REGISTRY_MAX)
		return telemetry_config_state_outcome::identity_registry_full;
	telemetry_config_identity_entry &entry = state.identities[free_index];
	entry = {};
	entry.used = 1U;
	entry.config_id = snapshot.config_id;
	std::memcpy(entry.fingerprint, snapshot.fingerprint, sizeof(entry.fingerprint));
	entry.canonical = snapshot;
	return telemetry_config_state_outcome::accepted;
}

int find_identity(const telemetry_config_state &state,
		  const telemetry_config_snapshot &snapshot) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_CONFIG_IDENTITY_REGISTRY_MAX; ++index)
	{
		const auto &entry = state.identities[index];
		if (entry.used == 0U)
			continue;
		const bool same_id = entry.config_id == snapshot.config_id;
		const bool same_fingerprint = bytes_equal(entry.fingerprint, snapshot.fingerprint,
							  TELEMETRY_CONFIG_FINGERPRINT_BYTES);
		if (same_id && same_fingerprint)
			return static_cast<int>(index);
		if (same_id || same_fingerprint)
			return -2;
	}
	return -1;
}

int find_pending(const telemetry_config_state &state,
		 const telemetry_config_snapshot &snapshot) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_CONFIG_PENDING_MAX; ++index)
		if (state.pending[index].used != 0U &&
		    snapshot_identity_equal(state.pending[index].snapshot, snapshot))
			return static_cast<int>(index);
	return -1;
}

bool has_newer_pending(const telemetry_config_state &state) noexcept
{
	if (state.has_admitted == 0U)
		return state.pending_count != 0U;
	for (const telemetry_config_pending_entry &entry : state.pending)
		if (entry.used != 0U && entry.snapshot.revision > state.admitted.revision)
			return true;
	return false;
}

void refresh_visibility(telemetry_config_state &state) noexcept
{
	state.visible = state.has_admitted != 0U &&
					state.admitted.revision > state.visibility_floor_revision &&
					has_newer_pending(state) == false ?
				1U :
				0U;
}

void block_visibility(telemetry_config_state &state, telemetry_config_revision revision) noexcept
{
	if (revision < state.highest_revision)
		revision = state.highest_revision;
	if (revision > state.visibility_floor_revision)
		state.visibility_floor_revision = revision;
	state.visible = 0U;
}

telemetry_config_state_result
result_for(const telemetry_config_state &state, telemetry_config_state_outcome outcome,
	   const telemetry_config_snapshot *snapshot = nullptr) noexcept
{
	telemetry_config_state_result result{};
	result.outcome = outcome;
	result.status = telemetry_config_state_status(&state);
	result.visible = state.visible;
	result.pending_count = state.pending_count;
	if (snapshot != nullptr)
		result.snapshot = *snapshot;
	else if (state.visible != 0U)
		result.snapshot = state.admitted;
	return result;
}

void remove_pending(telemetry_config_state &state, std::size_t index) noexcept
{
	if (index >= TELEMETRY_CONFIG_PENDING_MAX || state.pending[index].used == 0U)
		return;
	for (std::size_t next = index + 1U; next < TELEMETRY_CONFIG_PENDING_MAX; ++next)
		state.pending[next - 1U] = state.pending[next];
	state.pending[TELEMETRY_CONFIG_PENDING_MAX - 1U] = {};
	if (state.pending_count != 0U)
		--state.pending_count;
}

void make_configuration_record(telemetry_config_pending_entry &entry) noexcept
{
	entry.record = {};
	entry.record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	entry.record.header.kind = telemetry_record_kind::configuration;
	entry.record.header.reserved = 0U;
	entry.record.header.key = entry.key;
	entry.record.header.occurrence_utc_usec = entry.canonical.effective_utc_usec;
	entry.record.payload.configuration.config = entry.canonical;
}

telemetry_config_state_result attempt_pending(telemetry_config_state &state,
					      std::size_t index) noexcept
{
	if (index >= TELEMETRY_CONFIG_PENDING_MAX || state.pending[index].used == 0U)
		return result_for(state, telemetry_config_state_outcome::invalid);

	telemetry_config_pending_entry &entry = state.pending[index];
	if (state.key_allocator.next == nullptr || state.sink.emit == nullptr)
	{
		refresh_visibility(state);
		return result_for(state, telemetry_config_state_outcome::missing_context,
				  &entry.snapshot);
	}
	if (entry.has_key == 0U)
	{
		telemetry_record_key key{};
		if (!state.key_allocator.next(state.key_allocator.context,
					      telemetry_record_kind::configuration, &key) ||
		    !telemetry_record_key_is_valid(key))
		{
			refresh_visibility(state);
			return result_for(state,
					  telemetry_config_state_outcome::allocator_exhausted,
					  &entry.snapshot);
		}
		entry.key = key;
		entry.has_key = 1U;
		make_configuration_record(entry);
	}

	telemetry_config_state_result result{};
	result.admission_attempted = 1U;
	result.snapshot = entry.snapshot;
	result.record_key = entry.key;
	if (!state.sink.emit(state.sink.context, &entry.record))
	{
		saturating_increment(state.publication_rejections);
		refresh_visibility(state);
		result = result_for(state, telemetry_config_state_outcome::sink_rejected,
				    &entry.snapshot);
		result.admission_attempted = 1U;
		result.record_key = entry.key;
		return result;
	}

	saturating_increment(state.publication_accepts);
	const telemetry_record_key accepted_key = entry.key;
	const telemetry_config_snapshot accepted_snapshot = entry.snapshot;
	const telemetry_config_snapshot canonical_snapshot = entry.canonical;
	for (telemetry_config_identity_entry &identity : state.identities)
		if (identity.used != 0U && identity.config_id == canonical_snapshot.config_id &&
		    bytes_equal(identity.fingerprint, canonical_snapshot.fingerprint,
				TELEMETRY_CONFIG_FINGERPRINT_BYTES))
			identity.admitted = 1U;
	if (state.has_admitted == 0U || accepted_snapshot.revision >= state.admitted.revision)
	{
		state.admitted = accepted_snapshot;
		state.has_admitted = 1U;
	}
	remove_pending(state, index);
	refresh_visibility(state);
	result = result_for(state, telemetry_config_state_outcome::accepted);
	result.admission_attempted = 1U;
	result.admission_accepted = 1U;
	result.record_key = accepted_key;
	result.snapshot = accepted_snapshot;
	return result;
}

telemetry_config_state_result commit_disabled(telemetry_config_state &state,
					      telemetry_config_snapshot snapshot) noexcept
{
	for (telemetry_config_identity_entry &identity : state.identities)
		if (identity.used != 0U && identity.config_id == snapshot.config_id &&
		    bytes_equal(identity.fingerprint, snapshot.fingerprint,
				TELEMETRY_CONFIG_FINGERPRINT_BYTES))
			identity.admitted = 1U;
	if (state.has_admitted == 0U || snapshot.revision >= state.admitted.revision)
	{
		state.admitted = snapshot;
		state.has_admitted = 1U;
	}
	refresh_visibility(state);
	const telemetry_config_state_outcome outcome =
		snapshot.backend == telemetry_storage_backend::flatfile_disabled ?
			telemetry_config_state_outcome::flatfile_disabled :
			telemetry_config_state_outcome::disabled;
	return result_for(state, outcome, &snapshot);
}

telemetry_capture_result runtime_result(const telemetry_config_state_result &source) noexcept
{
	telemetry_capture_result result{};
	result.admission = telemetry_queue_admission::rejected_control_full;
	result.quality_flags = TELEMETRY_QUALITY_NONE;
	switch (source.outcome)
	{
	case telemetry_config_state_outcome::accepted:
		result.outcome = telemetry_runtime_outcome::accepted;
		result.admission = telemetry_queue_admission::accepted_control_reserve;
		result.records_emitted = 1U;
		result.first_record = source.record_key;
		result.last_record = source.record_key;
		break;
	case telemetry_config_state_outcome::unchanged:
		result.outcome = telemetry_runtime_outcome::accepted;
		result.admission = telemetry_queue_admission::accepted_control_reserve;
		break;
	case telemetry_config_state_outcome::disabled:
		result.outcome = telemetry_runtime_outcome::disabled;
		result.admission = telemetry_queue_admission::rejected_disabled;
		result.quality_flags = TELEMETRY_QUALITY_DISABLED;
		break;
	case telemetry_config_state_outcome::flatfile_disabled:
		result.outcome = telemetry_runtime_outcome::flatfile_disabled;
		result.admission = telemetry_queue_admission::rejected_disabled;
		result.quality_flags = TELEMETRY_QUALITY_DISABLED;
		break;
	case telemetry_config_state_outcome::uninitialized:
		result.outcome = telemetry_runtime_outcome::not_initialized;
		result.admission = telemetry_queue_admission::rejected_disabled;
		result.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
		break;
	case telemetry_config_state_outcome::invalid:
	case telemetry_config_state_outcome::stale_revision:
	case telemetry_config_state_outcome::identity_collision:
	case telemetry_config_state_outcome::revision_conflict:
		result.outcome = telemetry_runtime_outcome::invalid;
		result.admission = telemetry_queue_admission::rejected_invalid;
		result.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
		break;
	case telemetry_config_state_outcome::sink_rejected:
	case telemetry_config_state_outcome::allocator_exhausted:
	case telemetry_config_state_outcome::capacity_full:
	case telemetry_config_state_outcome::identity_registry_full:
	case telemetry_config_state_outcome::missing_context:
		result.outcome = telemetry_runtime_outcome::queue_full;
		result.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
		break;
	}
	return result;
}

} // namespace

const telemetry_config_property_definition *
telemetry_config_property_registry(std::size_t *count) noexcept
{
	if (count != nullptr)
		*count = PROPERTY_REGISTRY_COUNT;
	return PROPERTY_REGISTRY;
}

bool telemetry_config_property_name_is_allowlisted(const char *name) noexcept
{
	if (name == nullptr)
		return false;
	for (const telemetry_config_property_definition &definition : PROPERTY_REGISTRY)
	{
		if (std::strcmp(name, definition.stable_name) == 0 ||
		    (definition.source_key != nullptr &&
		     std::strcmp(name, definition.source_key) == 0))
			return true;
	}
	return false;
}

telemetry_config_property_snapshot telemetry_config_property_snapshot_defaults(void) noexcept
{
	telemetry_config_property_snapshot snapshot{};
	telemetry_config_property_capture capture{};
	capture.mode = telemetry_config_property_capture_mode::declared_defaults;
	(void)telemetry_config_property_snapshot_capture(&capture, &snapshot);
	return snapshot;
}

telemetry_config_build_outcome
telemetry_config_property_snapshot_capture(const telemetry_config_property_capture *capture,
					   telemetry_config_property_snapshot *snapshot) noexcept
{
	if (capture == nullptr || snapshot == nullptr ||
	    !property_capture_reserved_is_zero(*capture) ||
	    (capture->mode != telemetry_config_property_capture_mode::require_reader &&
	     capture->mode != telemetry_config_property_capture_mode::declared_defaults))
		return telemetry_config_build_outcome::invalid_input;
	*snapshot = {};
	snapshot->schema_version = TELEMETRY_CONFIG_PROPERTY_SCHEMA_VERSION;
	snapshot->count = static_cast<std::uint16_t>(PROPERTY_REGISTRY_COUNT);
	for (std::size_t index = 0U; index < PROPERTY_REGISTRY_COUNT; ++index)
	{
		const telemetry_config_property_definition &definition = PROPERTY_REGISTRY[index];
		telemetry_config_property_entry &entry = snapshot->entries[index];
		entry.id = definition.id;
		entry.role = static_cast<std::uint8_t>(definition.role);
		entry.kind = static_cast<std::uint8_t>(definition.kind);
		switch (definition.role)
		{
		case telemetry_config_property_role::maintained_but_unused:
			entry.source = static_cast<std::uint8_t>(
				telemetry_config_property_source::excluded_unused);
			entry.value = 0U;
			continue;
		case telemetry_config_property_role::unavailable_context:
			entry.source = static_cast<std::uint8_t>(
				telemetry_config_property_source::unavailable);
			entry.value = 0U;
			continue;
		case telemetry_config_property_role::hardcoded_effective:
			entry.source = static_cast<std::uint8_t>(
				telemetry_config_property_source::hardcoded);
			entry.value = definition.default_value;
			continue;
		case telemetry_config_property_role::effective:
			break;
		}
		if (definition.source_key == nullptr ||
		    definition.kind == telemetry_config_property_kind::unknown)
			return telemetry_config_build_outcome::property_registry_invalid;
		float value = 0.0f;
		if (capture->reader.read != nullptr)
		{
			if (!capture->reader.read(capture->reader.context, definition.source_key,
						  &value))
			{
				if (capture->mode ==
				    telemetry_config_property_capture_mode::require_reader)
					return telemetry_config_build_outcome::
						missing_property_context;
				entry.source = static_cast<std::uint8_t>(
					telemetry_config_property_source::declared_default);
				entry.value = definition.default_value;
				continue;
			}
			entry.source = static_cast<std::uint8_t>(
				telemetry_config_property_source::loaded_property);
		}
		else
		{
			if (capture->mode == telemetry_config_property_capture_mode::require_reader)
				return telemetry_config_build_outcome::missing_property_context;
			entry.source = static_cast<std::uint8_t>(
				telemetry_config_property_source::declared_default);
			entry.value = definition.default_value;
			continue;
		}
		if (!float_to_typed_bits(definition, value, entry.value))
			return telemetry_config_build_outcome::property_registry_invalid;
	}
	snapshot->property_version = property_version_for(*snapshot);
	property_digest_for(*snapshot, snapshot->effective_digest);
	if (!property_snapshot_is_valid(*snapshot))
		return telemetry_config_build_outcome::property_registry_invalid;
	if (capture->catalog.resolve == nullptr)
		return telemetry_config_build_outcome::built;
	telemetry_config_property_catalog_resolution resolution{};
	if (!capture->catalog.resolve(capture->catalog.context, snapshot,
				      snapshot->effective_digest, &resolution) ||
	    resolution.property_version == 0U || resolution.stable_namespace == 0U ||
	    resolution.stable_version == 0U ||
	    !bytes_equal(resolution.digest, snapshot->effective_digest,
			 TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES))
		return telemetry_config_build_outcome::property_registry_invalid;
	snapshot->property_version = resolution.property_version;
	snapshot->stable_namespace = resolution.stable_namespace;
	snapshot->stable_version = resolution.stable_version;
	return property_snapshot_catalog_is_valid(*snapshot) ?
		       telemetry_config_build_outcome::built :
		       telemetry_config_build_outcome::property_registry_invalid;
}

std::size_t telemetry_config_canonical_encode(const telemetry_config_snapshot &config,
					      std::uint8_t *bytes, std::size_t capacity) noexcept
{
	if (bytes == nullptr || capacity < TELEMETRY_CONFIG_CANONICAL_BYTES)
		return 0U;
	std::size_t offset = 0U;
	append_be(bytes, offset, config.schema_version, 2U);
	append_be(bytes, offset, config.build_version, 4U);
	append_be(bytes, offset, config.content_version, 4U);
	append_be(bytes, offset, config.property_version, 4U);
	append_be(bytes, offset, config.classifier_version, 4U);
	append_be(bytes, offset, config.policy_version, 4U);
	append_be(bytes, offset, config.season_id, 8U);
	append_be(bytes, offset, config.environment_id, 8U);
	append_be(bytes, offset, config.interval_usec, 8U);
	append_be(bytes, offset, config.checkpoint_interval_usec, 8U);
	append_be(bytes, offset, config.active_window_usec, 8U);
	append_be(bytes, offset, config.context_segments_per_minute, 4U);
	append_be(bytes, offset, config.pulse_slot_count, 2U);
	append_be(bytes, offset, static_cast<std::uint8_t>(config.backend), 1U);
	append_be(bytes, offset, config.enabled, 1U);
	return offset == TELEMETRY_CONFIG_CANONICAL_BYTES ? offset : 0U;
}

bool telemetry_config_compute_fingerprint(const telemetry_config_snapshot &config,
					  std::uint8_t *fingerprint, std::size_t capacity) noexcept
{
	if (fingerprint == nullptr || capacity < TELEMETRY_CONFIG_FINGERPRINT_BYTES)
		return false;
	std::uint8_t canonical[TELEMETRY_CONFIG_CANONICAL_BYTES]{};
	const std::size_t encoded =
		telemetry_config_canonical_encode(config, canonical, sizeof(canonical));
	if (encoded != sizeof(canonical))
		return false;
	return SHA256(canonical, encoded, fingerprint) != nullptr;
}

telemetry_config_id telemetry_config_id_from_fingerprint(const std::uint8_t *fingerprint,
							 std::size_t size) noexcept
{
	if (fingerprint == nullptr || size < 8U)
		return TELEMETRY_UNKNOWN_ID;
	std::uint64_t id = read_be64(fingerprint);
	if (id == TELEMETRY_UNKNOWN_ID && size >= 16U)
		id = read_be64(fingerprint + 8U);
	return id == TELEMETRY_UNKNOWN_ID ? 1U : id;
}

telemetry_config_build_outcome
telemetry_config_snapshot_build(const telemetry_config_capture_input *input,
				telemetry_config_property_snapshot *properties,
				telemetry_config_snapshot *snapshot) noexcept
{
	if (snapshot == nullptr)
		return telemetry_config_build_outcome::invalid_input;
	*snapshot = {};
	telemetry_config_property_snapshot local_properties{};
	telemetry_config_property_snapshot *captured = properties != nullptr ? properties :
									       &local_properties;
	*captured = {};
	if (input == nullptr || input->reserved != 0U ||
	    input->schema_version != TELEMETRY_SCHEMA_VERSION || input->revision == 0U ||
	    input->build_version == 0U || input->content_version == 0U ||
	    input->classifier_version == 0U || input->policy_version == 0U ||
	    input->season_id == TELEMETRY_UNKNOWN_ID ||
	    input->environment_id == TELEMETRY_UNKNOWN_ID ||
	    !telemetry_storage_backend_is_valid(input->backend) || input->enabled > 1U)
		return telemetry_config_build_outcome::invalid_input;
	const telemetry_config_build_outcome property_outcome =
		telemetry_config_property_snapshot_capture(&input->properties, captured);
	if (property_outcome != telemetry_config_build_outcome::built)
		return property_outcome;
	if (input->enabled != 0U && !property_snapshot_catalog_is_valid(*captured))
		return telemetry_config_build_outcome::property_catalog_unavailable;

	snapshot->schema_version = input->schema_version;
	snapshot->reserved = 0U;
	snapshot->revision = input->revision;
	snapshot->build_version = input->build_version;
	snapshot->content_version = input->content_version;
	snapshot->property_version = captured->property_version;
	snapshot->classifier_version = input->classifier_version;
	snapshot->policy_version = input->policy_version;
	snapshot->season_id = input->season_id;
	snapshot->environment_id = input->environment_id;
	snapshot->effective_utc_usec = input->effective_utc_usec;
	snapshot->interval_usec = input->interval_usec;
	snapshot->checkpoint_interval_usec = input->checkpoint_interval_usec;
	snapshot->active_window_usec = input->active_window_usec;
	snapshot->context_segments_per_minute = input->context_segments_per_minute;
	snapshot->pulse_slot_count = input->pulse_slot_count;
	snapshot->backend = input->backend;
	snapshot->enabled = input->enabled;
	if (!telemetry_config_compute_fingerprint(*snapshot, snapshot->fingerprint,
						  sizeof(snapshot->fingerprint)))
		return telemetry_config_build_outcome::invalid_input;
	snapshot->config_id = telemetry_config_id_from_fingerprint(snapshot->fingerprint,
								   sizeof(snapshot->fingerprint));
	return telemetry_config_is_valid(*snapshot) && snapshot_fingerprint_matches(*snapshot) ?
		       telemetry_config_build_outcome::built :
		       telemetry_config_build_outcome::invalid_input;
}

telemetry_config_state_outcome
telemetry_config_state_init(telemetry_config_state *state,
			    const telemetry_config_state_config *config) noexcept
{
	if (state == nullptr || config == nullptr)
		return telemetry_config_state_outcome::invalid;
	*state = {};
	state->initialized = 1U;
	state->sink = config->sink;
	state->key_allocator = config->key_allocator;
	return telemetry_config_state_outcome::accepted;
}

void telemetry_config_state_reset(telemetry_config_state *state) noexcept
{
	if (state != nullptr)
		*state = {};
}

telemetry_config_state_outcome
telemetry_config_state_seed_identity(telemetry_config_state *state,
				     const telemetry_config_snapshot &canonical) noexcept
{
	if (state == nullptr || state->initialized == 0U)
		return telemetry_config_state_outcome::uninitialized;
	if (state->publication_attempts != 0U || !telemetry_config_is_valid(canonical) ||
	    !snapshot_fingerprint_matches(canonical))
		return telemetry_config_state_outcome::invalid;
	return remember_identity(*state, canonical);
}

telemetry_config_state_result
telemetry_config_state_publish(telemetry_config_state *state,
			       telemetry_config_snapshot snapshot) noexcept
{
	if (state == nullptr || state->initialized == 0U)
	{
		telemetry_config_state_result result{};
		result.outcome = telemetry_config_state_outcome::uninitialized;
		result.status = telemetry_config_validation::missing_identity;
		return result;
	}
	saturating_increment(state->publication_attempts);
	if (!telemetry_config_is_valid(snapshot) || !snapshot_fingerprint_matches(snapshot))
	{
		block_visibility(*state, snapshot.revision);
		saturating_increment(state->publication_rejections);
		return result_for(*state, telemetry_config_state_outcome::invalid, &snapshot);
	}

	const int identity_index = find_identity(*state, snapshot);
	if (identity_index == -2)
	{
		block_visibility(*state, snapshot.revision);
		saturating_increment(state->publication_rejections);
		return result_for(*state, telemetry_config_state_outcome::identity_collision,
				  &snapshot);
	}
	const int pending_index = find_pending(*state, snapshot);
	if (pending_index >= 0)
	{
		telemetry_config_pending_entry &entry = state->pending[pending_index];
		if (snapshot.revision > entry.snapshot.revision &&
		    snapshot.revision <= state->highest_revision)
		{
			const auto outcome =
				snapshot.revision == state->highest_revision ?
					telemetry_config_state_outcome::revision_conflict :
					telemetry_config_state_outcome::stale_revision;
			if (outcome == telemetry_config_state_outcome::revision_conflict)
				block_visibility(*state, snapshot.revision);
			saturating_increment(state->publication_rejections);
			return result_for(*state, outcome, &snapshot);
		}
		if (snapshot.revision > entry.snapshot.revision)
		{
			entry.snapshot = snapshot;
			if (snapshot.revision > state->highest_revision)
				state->highest_revision = snapshot.revision;
		}
		return attempt_pending(*state, 0U);
	}
	const bool known_identity = identity_index >= 0;
	const bool same_current = state->has_admitted != 0U &&
				  snapshot_identity_equal(state->admitted, snapshot);
	if (state->highest_revision != 0U && snapshot.revision < state->highest_revision &&
	    !same_current)
	{
		saturating_increment(state->publication_rejections);
		return result_for(*state, telemetry_config_state_outcome::stale_revision,
				  &snapshot);
	}
	if (state->highest_revision != 0U && snapshot.revision == state->highest_revision &&
	    !same_current)
	{
		block_visibility(*state, snapshot.revision);
		saturating_increment(state->publication_rejections);
		return result_for(*state, telemetry_config_state_outcome::revision_conflict,
				  &snapshot);
	}
	if (known_identity)
	{
		if (snapshot.revision > state->highest_revision)
			state->highest_revision = snapshot.revision;
		if (same_current)
		{
			if (snapshot.revision > state->admitted.revision)
				state->admitted = snapshot;
			if (snapshot.enabled == 0U)
				return commit_disabled(*state, snapshot);
			refresh_visibility(*state);
			return result_for(*state, telemetry_config_state_outcome::unchanged,
					  &snapshot);
		}
	}
	else
	{
		const telemetry_config_state_outcome identity_outcome =
			remember_identity(*state, snapshot);
		if (identity_outcome != telemetry_config_state_outcome::accepted)
		{
			block_visibility(*state, snapshot.revision);
			saturating_increment(state->publication_rejections);
			return result_for(*state, identity_outcome, &snapshot);
		}
	}
	if (snapshot.revision > state->highest_revision)
		state->highest_revision = snapshot.revision;
	if (snapshot.enabled == 0U)
		return commit_disabled(*state, snapshot);

	if (state->pending_count >= TELEMETRY_CONFIG_PENDING_MAX)
	{
		block_visibility(*state, snapshot.revision);
		saturating_increment(state->publication_rejections);
		return result_for(*state, telemetry_config_state_outcome::capacity_full, &snapshot);
	}
	telemetry_config_pending_entry &entry = state->pending[state->pending_count];
	entry = {};
	entry.used = 1U;
	entry.snapshot = snapshot;
	const int canonical_index = find_identity(*state, snapshot);
	if (canonical_index < 0)
	{
		block_visibility(*state, snapshot.revision);
		saturating_increment(state->publication_rejections);
		return result_for(*state, telemetry_config_state_outcome::invalid, &snapshot);
	}
	entry.canonical = state->identities[canonical_index].canonical;
	++state->pending_count;
	return attempt_pending(*state, 0U);
}

telemetry_config_snapshot
telemetry_config_state_snapshot_copy(const telemetry_config_state *state) noexcept
{
	if (state == nullptr || state->initialized == 0U || state->visible == 0U)
		return {};
	return state->admitted;
}

telemetry_config_validation
telemetry_config_state_status(const telemetry_config_state *state) noexcept
{
	if (state == nullptr || state->initialized == 0U || state->visible == 0U)
		return telemetry_config_validation::missing_identity;
	return telemetry_config_validate(state->admitted);
}

telemetry_config_state_stats
telemetry_config_state_stats_copy(const telemetry_config_state *state) noexcept
{
	telemetry_config_state_stats stats{};
	stats.capacity = static_cast<std::uint16_t>(TELEMETRY_CONFIG_PENDING_MAX);
	if (state == nullptr)
		return stats;
	stats.pending_count = state->pending_count;
	stats.initialized = state->initialized;
	stats.has_admitted = state->has_admitted;
	stats.visible = state->visible;
	stats.reload_requested = state->reload_requested;
	stats.publication_attempts = state->publication_attempts;
	stats.publication_accepts = state->publication_accepts;
	stats.publication_rejections = state->publication_rejections;
	return stats;
}

void telemetry_config_reload_observer(void *context) noexcept
{
	if (context == nullptr)
		return;
	telemetry_config_state &state = *static_cast<telemetry_config_state *>(context);
	state.reload_requested = 1U;
	block_visibility(state, state.highest_revision);
	for (telemetry_config_pending_entry &entry : state.pending)
		entry = {};
	state.pending_count = 0U;
}

bool telemetry_config_reload_requested(const telemetry_config_state *state) noexcept
{
	return state != nullptr && state->reload_requested != 0U;
}

void telemetry_config_reload_request_clear(telemetry_config_state *state) noexcept
{
	if (state != nullptr)
		state->reload_requested = 0U;
}

telemetry_config_state_outcome
telemetry_config_global_init(const telemetry_config_state_config *config) noexcept
{
	return telemetry_config_state_init(&GLOBAL_STATE, config);
}

telemetry_config_state *telemetry_config_global_state(void) noexcept
{
	return &GLOBAL_STATE;
}

void telemetry_config_global_reset(void) noexcept
{
	telemetry_config_state_reset(&GLOBAL_STATE);
}

telemetry_config_snapshot telemetry_config_snapshot_copy(void)
{
	return telemetry_config_state_snapshot_copy(&GLOBAL_STATE);
}

telemetry_config_validation telemetry_config_status(void)
{
	return telemetry_config_state_status(&GLOBAL_STATE);
}

telemetry_capture_result telemetry_config_publish(telemetry_config_snapshot config)
{
	return runtime_result(telemetry_config_state_publish(&GLOBAL_STATE, config));
}
