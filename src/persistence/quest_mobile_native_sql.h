#ifndef QUEST_MOBILE_NATIVE_SQL_H
#define QUEST_MOBILE_NATIVE_SQL_H

#include "world/quest_mobile_native.h"
#ifdef __NO_MYSQL__
#include "no_mysql/mysql.h"
#else
#include <mysql/mysql.h>
#endif

// Value/session observation, not a birth/source/epoch/inbox authority or ACK.
// An absent row retains only the exact requested ID/session; image is empty.
struct quest_mobile_native_sql_row
{
	uint64_t mobile_instance_id = 0;
	unsigned long original_session = 0;
	bool present = false;
	quest_mobile_native_image image;
};
// Caller owns one original reconnect-disabled IN_TRANS session, canonical
// authority/inbox/exclusion and ascending mobile-ID locks BEFORE item custody.
// Never starts, commits, rolls back, replaces a session, issues IDs or touches world.
// Return 0 on exact value proof; otherwise errno/native SQL code, output unchanged.
int quest_mobile_native_sql_lock(MYSQL *, uint64_t mobile_instance_id,
				 quest_mobile_native_sql_row *) noexcept;
// Rechecks the complete original before under the same lock/session, then uses
// binary prepared DML and exact after readback. Parent owns typed admitted
// transition/increment validation and the entire item-custody atomic operation.
// Birth requires actual missing locked row and original parent == birth operation;
// bare values/session never authenticate an admitted birth. Existing birth facts
// cannot change and RETIRED cannot revive.
// Cash-aware birth and ordinary transitions require known v2 cash with the
// checked cash/mobile revision policy. Historical unknown cash is read-only here.
// Failure after DML requires caller transaction rollback/retirement;
// output preservation is not native rollback.
int quest_mobile_native_sql_apply_locked(MYSQL *, const critical_operation_id &parent,
					 const quest_mobile_native_sql_row &original_before,
					 const quest_mobile_native_image &after,
					 quest_mobile_native_sql_row *verified_after) noexcept;
#endif
