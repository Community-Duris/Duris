#include "guild/artifact_guild_command.h"
#include "guild/artifact_guild_repository.h"

#include <cassert>
#include <cerrno>
#include <climits>
#include <vector>

int main()
{
	critical_operation_id parent = {};
	parent.bytes[0] = 0x55;
	parent.bytes[15] = 0x66;

	critical_operation_id op = {};
	assert(critical_operation_id_derive(parent, 0x41475431, 1, &op));

	artifact_guild_payload payload = {};
	payload.parent_operation_id = parent;
	payload.actor_pid = 12345;
	payload.guild_id = 99;
	payload.expected_guild_revision = 5;
	payload.prestige_delta = 100;
	payload.construction_delta = 50;
	payload.artifact_count = 1;
	payload.artifacts[0] = {
		.vnum = 7001,
		.flags = ARTIFACT_DELTA_FEED | ARTIFACT_DELTA_BIND,
		.expected_revision = 1,
		.expected_timer = 1000,
		.timer = 2000,
		.expected_bind_owner_pid = 0,
		.bind_owner_pid = 12345,
		.expected_bind_timer = 0,
		.bind_timer = 500,
	};

	critical_command command = {};
	assert(artifact_guild_command_build(&command, op, payload));
	assert(command.type == critical_command_type::artifact);

	artifact_guild_result result = {};
	unsigned int result_code = 0;
	bool mutation_applied = false;

#ifdef __NO_MYSQL__
	errno = 0;
	bool ok = artifact_guild_repository_execute(nullptr, command, &result, &result_code,
						    &mutation_applied);
	assert(!ok);
	assert(errno == ENOTSUP);
	assert(!mutation_applied);
#endif

	return 0;
}
