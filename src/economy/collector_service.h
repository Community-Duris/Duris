#ifndef DURIS_COLLECTOR_SERVICE_H
#define DURIS_COLLECTOR_SERVICE_H

#include "core/structs.h"

#include <cstddef>
#include <cstdint>

struct collector_service_health
{
	size_t pending_details = 0;
	uint64_t submitted_details = 0;
	uint64_t completed_details = 0;
	uint64_t abandoned_offline = 0;
	uint64_t stale_results = 0;
	uint64_t rejected_details = 0;
	uint64_t submitted_purchases = 0;
	uint64_t rejected_purchases = 0;
	uint64_t committed_purchases = 0;
	uint64_t materialization_failures = 0;
};

struct critical_command;
// Register only the original accounted SQL purchase during prepared startup.
// A genuine later coordinator completion supplies the seal; this never derives
// one from the result-only outbox or publishes before native proof/guarded ACK.
bool collector_service_restore_replayed_purchase(const critical_command &) noexcept;

void collector_service_command(P_char character, char *arguments, int command);
void collector_service_pulse(void);
// Reconcile a player-load/reconnect after the inventory graph is hydrated.
// A reconnect may retry retained payloads, but only a cold load may clear the
// per-character fence left when no payload survived.
void collector_service_player_ready(P_char character, bool inventory_reloaded);
bool collector_service_player_save_fenced(P_char character);
// Retry committed purchase materialization before a character is serialized.
// A false result means the save must remain deferred so player_items cannot be
// overwritten without the durable purchase in the live graph.
bool collector_service_recover_player(P_char character);
bool collector_service_player_busy(P_char character);
collector_service_health collector_service_health_copy(void);
void collector_service_reset_for_tests(void);

class player_save_coin_replay_budget_scope_owner;
bool collector_service_restore_replayed_purchase_bounded(
	const critical_command &, player_save_coin_replay_budget_scope_owner &,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live) noexcept;
size_t collector_service_replay_observer_frame_bytes() noexcept;
bool collector_service_replay_current_storage_bytes(size_t *) noexcept;

#endif
