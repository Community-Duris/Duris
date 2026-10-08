#include "player/player_save_pipeline.h"
#include "player/player_snapshot_repository.h"
#include "item/ordinary_drop_recovery.h"
#include "economy/coin_physical_recovery.h"
#include <cstdio>
#include <cstdlib>

// These are abort-only link boundaries, never successful authority. The stopped
// replay fixture has no enabled ownership epoch; the real reservation refuses
// before covered-revision observation, native publication, or guarded ACK.
[[noreturn]] static void forbidden(const char *boundary)
{
	std::fprintf(stderr, "UNREACHABLE_NATIVE_BOUNDARY_CALLED %s\n", boundary);
	std::abort();
}

bool critical_command_coordinator_acknowledge_publication(player_save_restored_publication_owner &)
{
	forbidden("guarded_critical_ack");
}

bool player_snapshot_repository_observe_covered_revision(
	int, const player_save_execution_guard::held_publication_reservation &,
	player_save_covered_revision *) noexcept
{
	forbidden("covered_native_revision");
}

ordinary_drop_observation ordinary_drop_recovery_publish(const critical_command &,
							 const critical_completion &) noexcept
{
	forbidden("ordinary_native_publication");
}

// The actual coin recovery TU remains linked for its unchanged identity/shape
// decoder. GNU ld wraps only the unreachable native publication entry.
bool forbidden_coin_native_publication(const critical_command &,
				       const critical_completion &) noexcept
	asm("__wrap__Z30coin_physical_recovery_publishRK16critical_commandRK19critical_completion");
bool forbidden_coin_native_publication(const critical_command &,
				       const critical_completion &) noexcept
{
	forbidden("coin_native_publication");
}
