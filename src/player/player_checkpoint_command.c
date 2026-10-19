#include "player/player_checkpoint_command.h"
#include "player/player_snapshot_codec.h"
#include <algorithm>
#include <cerrno>
#include <chrono>

namespace
{
std::vector<uint8_t> receipt(const player_snapshot &snapshot)
{
	std::vector<uint8_t> bytes;
	for (size_t index = 0; index < 4; ++index)
		bytes.push_back(
			static_cast<uint8_t>(static_cast<uint32_t>(snapshot.pid) >> (index * 8)));
	for (size_t index = 0; index < 8; ++index)
		bytes.push_back(static_cast<uint8_t>(snapshot.revision >> (index * 8)));
	return bytes;
}
}

bool player_checkpoint_command_build(const player_snapshot &snapshot, critical_command *command)
{
	if (!command || snapshot.pid <= 0 || !snapshot.revision)
		return false;
	critical_command built{};
	built.schema_version = CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION;
	built.type = critical_command_type::player_checkpoint;
	built.payload_version = 1;
	built.source_site = critical_source_site::command;
	built.deadline_class = critical_deadline_class::background;
	built.accepted_at_usec =
		static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					      std::chrono::system_clock::now().time_since_epoch())
					      .count());
	if (!critical_operation_id_generate(&built.operation_id) ||
	    player_snapshot_encode(snapshot, &built.payload) != player_snapshot_codec_result::ok)
		return false;
	built.keys.push_back({ critical_entity_type::player, static_cast<uint64_t>(snapshot.pid) });
	built.outcome_revision = snapshot.revision;
	built.outcome_payload = receipt(snapshot);
	if (!critical_command_valid(built))
		return false;
	*command = std::move(built);
	return true;
}

critical_apply_result player_checkpoint_command_project(const critical_command &command,
							player_save_apply_fn apply, void *context)
{
	player_snapshot snapshot{};
	if (!apply || command.type != critical_command_type::player_checkpoint ||
	    command.schema_version != CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION ||
	    command.payload_version != 1 || !critical_command_valid(command) ||
	    player_snapshot_decode(command.payload.data(), command.payload.size(), &snapshot) !=
		    player_snapshot_codec_result::ok ||
	    command.keys.size() != 1 || command.keys[0].type != critical_entity_type::player ||
	    command.keys[0].id != static_cast<uint64_t>(snapshot.pid) ||
	    command.outcome_revision != snapshot.revision ||
	    command.outcome_payload != receipt(snapshot))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	struct epoch_scope
	{
		uint64_t previous = player_checkpoint_projecting_epoch;
		~epoch_scope() { player_checkpoint_projecting_epoch = previous; }
	} scope;
	player_checkpoint_projecting_epoch = command.accepted_at_usec / 1000000;
	const auto applied = apply(snapshot, context);
	if (applied.outcome == player_save_apply_outcome::retryable_failure ||
	    applied.outcome == player_save_apply_outcome::ambiguous_commit)
		return { critical_apply_outcome::retryable_failure, 0, applied.error_code };
	if ((applied.outcome != player_save_apply_outcome::applied &&
	     applied.outcome != player_save_apply_outcome::already_applied &&
	     applied.outcome != player_save_apply_outcome::stale_revision) ||
	    applied.durable_revision < snapshot.revision)
		return { critical_apply_outcome::terminal_failure, 0,
			 applied.error_code ? applied.error_code : EILSEQ };
	critical_apply_result result{ critical_apply_outcome::applied, snapshot.revision, 0 };
	result.result_size = command.outcome_payload.size();
	std::copy(command.outcome_payload.begin(), command.outcome_payload.end(),
		  result.result_payload.begin());
	return result;
}
