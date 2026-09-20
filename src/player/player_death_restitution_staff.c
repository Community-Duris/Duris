#include "player/player_death_restitution_staff.h"
#include "core/config.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <vector>

namespace
{
constexpr size_t MAX_STAGED_STAFF_SUBMISSIONS = 4;
constexpr size_t MAX_STAFF_ACTOR_BYTES = 128;

struct staged_staff_submission
{
	bool active = false;
	std::string actor;
	std::string hex;
	size_t chunk_count = 0;
};

// All access is on the game thread through the staff command path.  A fixed
// slot count and the codec bound prevent an operator from turning staging into
// an unbounded allocation source.
std::array<staged_staff_submission, MAX_STAGED_STAFF_SUBMISSIONS> staged_submissions = {};

bool actor_authorized(const char *actor, int actor_level)
{
	return actor && *actor && actor_level >= FORGER &&
	       strnlen(actor, MAX_STAFF_ACTOR_BYTES + 1) <= MAX_STAFF_ACTOR_BYTES;
}

staged_staff_submission *find_staged(const char *actor)
{
	if (!actor)
		return nullptr;
	for (auto &staged : staged_submissions)
		if (staged.active && staged.actor == actor)
			return &staged;
	return nullptr;
}

staged_staff_submission *allocate_staged(const char *actor)
{
	if (staged_staff_submission *existing = find_staged(actor))
		return existing;
	for (auto &staged : staged_submissions)
		if (!staged.active)
		{
			staged.active = true;
			staged.actor = actor;
			return &staged;
		}
	return nullptr;
}

int hex_digit(unsigned char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

bool result_keeps_operation(player_death_restitution_runtime_result result)
{
	return result == player_death_restitution_runtime_result::accepted ||
	       result == player_death_restitution_runtime_result::awaiting_durability ||
	       result == player_death_restitution_runtime_result::attached ||
	       result == player_death_restitution_runtime_result::journal_uncertain;
}

bool hex_chunk_valid(const char *input, size_t input_size)
{
	if (!input || input_size == 0 || input_size > PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNK_HEX)
		return false;
	for (size_t index = 0; index < input_size; ++index)
		if (hex_digit(static_cast<unsigned char>(input[index])) < 0)
			return false;
	return true;
}

bool decode_hex(const char *input, size_t input_size, std::vector<uint8_t> *decoded)
{
	if (!input || !decoded || input_size == 0 || (input_size & 1) != 0 ||
	    input_size > PLAYER_DEATH_RESTITUTION_STAFF_MAX_COMMAND_BYTES * 2)
		return false;
	try
	{
		decoded->clear();
		decoded->reserve(input_size / 2);
		for (size_t index = 0; index < input_size; index += 2)
		{
			const int high = hex_digit(static_cast<unsigned char>(input[index]));
			const int low = hex_digit(static_cast<unsigned char>(input[index + 1]));
			if (high < 0 || low < 0)
				return false;
			decoded->push_back(static_cast<uint8_t>((high << 4) | low));
		}
	}
	catch (const std::bad_alloc &)
	{
		decoded->clear();
		return false;
	}
	return true;
}
} // namespace

player_death_restitution_runtime_result player_death_restitution_staff_submit_hex(
	const char *actor, int actor_level, const char *canonical_hex, size_t canonical_hex_size,
	player_death_restitution_runtime_submission *submission_out)
{
	if (submission_out)
		*submission_out = {};
	if (!actor_authorized(actor, actor_level))
		return player_death_restitution_runtime_result::unauthorized;

	std::vector<uint8_t> encoded;
	if (!decode_hex(canonical_hex, canonical_hex_size, &encoded))
		return player_death_restitution_runtime_result::invalid_plan;

	critical_command command = {};
	if (critical_command_decode(encoded.data(), encoded.size(), &command) !=
		    critical_command_codec_result::ok ||
	    command.type != critical_command_type::player_death_restitution ||
	    command.source_site != critical_source_site::operator_repair ||
	    command.deadline_class != critical_deadline_class::interactive)
		return player_death_restitution_runtime_result::invalid_plan;

	// A canonical handoff must round-trip byte-for-byte.  This rejects an
	// otherwise decodable but non-canonical representation before any fence or
	// coordinator state is touched.
	std::vector<uint8_t> canonical;
	if (critical_command_encode(command, &canonical) != critical_command_codec_result::ok ||
	    canonical != encoded)
		return player_death_restitution_runtime_result::invalid_plan;

	return player_death_restitution_runtime_submit_live_approved(command, actor, actor_level,
								     submission_out);
}

player_death_restitution_runtime_result player_death_restitution_staff_begin(const char *actor,
									     int actor_level)
{
	if (!actor_authorized(actor, actor_level))
		return player_death_restitution_runtime_result::unauthorized;
	try
	{
		staged_staff_submission *staged = find_staged(actor);
		if (staged)
			return player_death_restitution_runtime_result::duplicate_staging;
		staged = allocate_staged(actor);
		if (!staged)
			return player_death_restitution_runtime_result::overloaded;
		staged->hex.clear();
		staged->chunk_count = 0;
	}
	catch (const std::bad_alloc &)
	{
		return player_death_restitution_runtime_result::overloaded;
	}
	return player_death_restitution_runtime_result::accepted;
}

player_death_restitution_runtime_result
player_death_restitution_staff_append_hex(const char *actor, int actor_level, const char *hex_chunk,
					  size_t hex_chunk_size)
{
	if (!actor_authorized(actor, actor_level))
		return player_death_restitution_runtime_result::unauthorized;
	staged_staff_submission *staged = find_staged(actor);
	if (!staged)
		return player_death_restitution_runtime_result::no_staging;
	if (!hex_chunk || hex_chunk_size == 0)
		return player_death_restitution_runtime_result::malformed_chunk;
	if (hex_chunk_size > PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNK_HEX ||
	    staged->chunk_count >= PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNKS ||
	    staged->hex.size() > PLAYER_DEATH_RESTITUTION_STAFF_MAX_COMMAND_BYTES * 2 ||
	    hex_chunk_size >
		    PLAYER_DEATH_RESTITUTION_STAFF_MAX_COMMAND_BYTES * 2 - staged->hex.size())
		return player_death_restitution_runtime_result::chunk_limit;
	if ((hex_chunk_size & 1) != 0)
		return player_death_restitution_runtime_result::incomplete_chunk;
	if (!hex_chunk_valid(hex_chunk, hex_chunk_size))
		return player_death_restitution_runtime_result::malformed_chunk;
	try
	{
		staged->hex.append(hex_chunk, hex_chunk_size);
		++staged->chunk_count;
	}
	catch (const std::bad_alloc &)
	{
		return player_death_restitution_runtime_result::overloaded;
	}
	return player_death_restitution_runtime_result::accepted;
}

player_death_restitution_runtime_result
player_death_restitution_staff_commit(const char *actor, int actor_level,
				      player_death_restitution_runtime_submission *submission_out)
{
	if (submission_out)
		*submission_out = {};
	if (!actor_authorized(actor, actor_level))
		return player_death_restitution_runtime_result::unauthorized;
	staged_staff_submission *staged = find_staged(actor);
	if (!staged)
		return player_death_restitution_runtime_result::no_staging;
	if (staged->hex.empty() || (staged->hex.size() & 1) != 0)
		return player_death_restitution_runtime_result::incomplete_chunk;

	std::string canonical_hex;
	try
	{
		canonical_hex = staged->hex;
	}
	catch (const std::bad_alloc &)
	{
		return player_death_restitution_runtime_result::overloaded;
	}
	const player_death_restitution_runtime_result result =
		player_death_restitution_staff_submit_hex(actor, actor_level, canonical_hex.data(),
							  canonical_hex.size(), submission_out);
	if (result_keeps_operation(result))
		*staged = {};
	return result;
}

player_death_restitution_runtime_result player_death_restitution_staff_abort(const char *actor,
									     int actor_level)
{
	if (!actor_authorized(actor, actor_level))
		return player_death_restitution_runtime_result::unauthorized;
	staged_staff_submission *staged = find_staged(actor);
	if (!staged)
		return player_death_restitution_runtime_result::no_staging;
	*staged = {};
	return player_death_restitution_runtime_result::accepted;
}

bool player_death_restitution_staff_get_staging_status(
	const char *actor, int actor_level,
	player_death_restitution_staff_staging_status *status_out)
{
	if (status_out)
		*status_out = {};
	if (!status_out || !actor_authorized(actor, actor_level))
		return false;
	staged_staff_submission *staged = find_staged(actor);
	if (!staged)
	{
		status_out->state = player_death_restitution_staff_staging_state::inactive;
		status_out->max_chunks = PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNKS;
		status_out->max_hex_bytes = PLAYER_DEATH_RESTITUTION_STAFF_MAX_COMMAND_BYTES * 2;
		return true;
	}
	status_out->state = player_death_restitution_staff_staging_state::active;
	status_out->accepted_chunks = staged->chunk_count;
	status_out->accepted_hex_bytes = staged->hex.size();
	status_out->max_chunks = PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNKS;
	status_out->max_hex_bytes = PLAYER_DEATH_RESTITUTION_STAFF_MAX_COMMAND_BYTES * 2;
	return true;
}
