#ifndef ZONE_RESET_ITEM_ORIGIN_SQL_H
#define ZONE_RESET_ITEM_ORIGIN_SQL_H

#include "economy/zone_reset_item_origin.h"
#ifdef __NO_MYSQL__
#include "no_mysql/mysql.h"
#else
#include <mysql/mysql.h>
#endif

// Caller owns the original reconnect-disabled IN_TRANS root. Call AFTER its
// successful inbox/outbox and full indexed evidence, BEFORE committing it.
// Authenticate the original receipt, then retain identical immutable bytes.
// No current room/custody/publication requirement, commit, retry or journal ACK.
int zone_reset_item_origin_sql_retain_locked(MYSQL *, const critical_command &,
					     std::span<const uint8_t> result) noexcept;

// Authenticate original root receipt BEFORE taking the immutable carrier lock,
// then compare the complete observed and locked bytes. Original room/season,
// quantities or custody may have progressed. Absent is unknown, never evidence
// to synthesize; a cold consumer independently proves current state and the
// complete forest before publication. Strong output on every refusal.
int zone_reset_item_origin_sql_lock(MYSQL *, const critical_operation_id &,
				    zone_reset_item_retained_origin *) noexcept;

#endif
