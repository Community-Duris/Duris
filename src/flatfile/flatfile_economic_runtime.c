#include "flatfile/flatfile_economic_runtime.h"
#include "flatfile/flatfile_accounting_lifecycle_transaction.h"
#include "flatfile/flatfile_identity_repository.h"
#include "economy/economic_gameplay_authority.h"
#include "persistence/persistence_mode.h"
#include "persistence/critical_command_coordinator.h"
#include "player/player_save_replay_ownership.h"
#include <string>
#include <unistd.h>

namespace
{
// Main-thread lifecycle state only, not native holdings or activation authority.
pid_t runtime_process = 0;
} // namespace

bool flatfile_economic_runtime_start() noexcept
{
	try
	{
		if (persistence_mode_requires_mysql() || runtime_process ||
		    economic_gameplay_authority::active())
			return false;
		const char *configured = persistence_mode_flatfile_root();
		if (!configured || !*configured)
			return false;
		const std::string root(configured);
		std::string error;
		flatfile_identity_lock identity;
		flatfile_authority_lock authority;
		// Exactly the native writer order; borrow these locks through verified
		// current mapping/native enumeration and final projection publication.
		if (!identity.acquire(root, &error) || !authority.acquire(root, &error))
			return false;
		bool active = false;
		if (flatfile_accounting_lifecycle_transaction::recover_runtime_locked(
			    root, identity, authority, &active, &error))
			return false;
		// The private owner publishes last; every failure leaves it absent.
		// No throwing work follows its successful publication.
		runtime_process = getpid();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void flatfile_economic_runtime_shutdown() noexcept
{
	// A frontend, forked child, failed save close or refused coordinator close
	// cannot release the main process's read-only admission projection.
	if (!runtime_process || runtime_process != getpid() || persistence_mode_requires_mysql() ||
	    !critical_command_coordinator_lifecycle_guard_held_by_current_thread() ||
	    player_save_execution_guard::current_ownership_epoch())
		return;
	const auto health = critical_command_coordinator_health_copy();
	if (health.initialized || health.running || health.admission_worker_running ||
	    health.append_inflight || health.shutdown_refused)
		return;
	economic_gameplay_authority::clear_flat_runtime();
	runtime_process = 0;
}
