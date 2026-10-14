#ifndef CORPSE_RAISE_DIAGNOSTICS_H
#define CORPSE_RAISE_DIAGNOSTICS_H

#include "persistence/critical_command.h"
#include <array>
#include <cstdlib>
#include <cstring>

// Optional isolated-test evidence only. This never changes an admission,
// publication, native outcome, recovery decision or ACK. Fixed on first use;
// the commissioned test process must inherit the flag before boot.
inline bool corpse_raise_diagnostics_enabled() noexcept
{
	static const bool enabled = []
	{
		const char *value = std::getenv("DURIS_TEST_CORPSE_RAISE_DIAGNOSTICS");
		return value && std::strcmp(value, "1") == 0;
	}();
	return enabled;
}

enum class corpse_raise_diagnostic_kind : uint32_t
{
	native_item = 1,
	owner_clock,
	legacy_discard_count,
	source_selection,
	owner_cache_item,
	native_actor,
	observation_exception,
};

enum class corpse_raise_command_capture_status : uint8_t
{
	empty,
	encoded,
	codec_refused,
	oversize,
	exception,
};

// Last PC raise's actual PRE-ADMISSION encoded critical_command, captured by
// its original transaction producer before moving that command to submit.
// accepted_at_usec is the genuine value at that stage, not a claimed journal
// record. SQL retains the separate exact result body. Older captures are
// overwritten explicitly; this is diagnostic storage, never replay input.
struct corpse_raise_command_observation
{
	uint64_t sequence = 0, overwritten = 0, corpse_key = 0;
	critical_operation_id operation = {};
	uint32_t schema_version = 0;
	uint16_t command_type = 0, payload_version = 0;
	critical_command_codec_result codec_result = critical_command_codec_result::invalid;
	corpse_raise_command_capture_status status = corpse_raise_command_capture_status::empty;
	size_t encoded_size = 0;
	std::array<uint8_t, CRITICAL_COMMAND_MAX_ENCODED_BYTES> encoded = {};
};

// Read-only view of producer-owned storage on the serialized game thread (or
// with all threads stopped for a read-only native snapshot). Null when disabled.
// The next original PC-raise submission replaces the view; no authority follows.
const corpse_raise_command_observation *corpse_raise_diagnostic_command_observe() noexcept;

#endif
