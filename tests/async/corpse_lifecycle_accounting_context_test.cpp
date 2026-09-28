#include "persistence/corpse_lifecycle_command.h"
#include "persistence/corpse_lifecycle_repository.h"

#include <cassert>

bool critical_command_repository_begin_inbox_in_transaction(MYSQL *, const critical_command &)
{
	return false;
}

bool critical_command_repository_finish_item_transfer_in_transaction(MYSQL *,
								     const critical_command &,
								     const item_transfer_result &)
{
	return false;
}

bool currency_repository_execute(MYSQL *, const critical_command &, currency_command_result *,
				 unsigned int *, bool *)
{
	return false;
}

int main()
{
	critical_command cmd = {};
	cmd.operation_id.bytes[0] = 0x55;

	corpse_lifecycle_result result = {};
	unsigned int result_code = 0;
	bool mutation = false;
	uint64_t collector_revision = 0;
	std::vector<collector_command_result> collector_events;

	// In NO_MYSQL mode, repository functions gracefully return false
	bool ok = corpse_lifecycle_repository_execute(nullptr, cmd, &result, &result_code,
						      &mutation, &collector_revision,
						      &collector_events);
	assert(!ok);

	return 0;
}
