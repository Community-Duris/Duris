#ifndef DURIS_COLLECTOR_NOTIFICATION_H
#define DURIS_COLLECTOR_NOTIFICATION_H

#include "economy/collector_policy.h"

#include <cstddef>
#include <cstdint>
#include <string>

// Build the player-facing recovery hint. The text deliberately names only the
// public Collector location and commands; it does not include a death room,
// item owner, or another player's identity.
bool collector_notification_format(const collector::record &entry, std::string *message);

// Called after an activation completion and from lifecycle retry points. The
// durable hint action performs the authoritative stale-listing check before the
// message is sent.
void collector_notification_on_available(const collector::record &entry);
void collector_notification_pulse(void);
void collector_notification_player_ready(void);
void collector_notification_death_enrolled(void);

// Test/lifecycle cleanup for the process-local delivery retry table. Durable
// hint state remains in the repository and is not cleared by this function.
void collector_notification_reset_for_tests(void);
size_t collector_notification_pending_count(void);

#endif
