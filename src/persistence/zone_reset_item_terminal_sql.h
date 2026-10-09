#ifndef ZONE_RESET_ITEM_TERMINAL_SQL_H
#define ZONE_RESET_ITEM_TERMINAL_SQL_H

#include "economy/zone_reset_item_recovery.h"
#include "persistence/zone_reset_item_origin_sql.h"

// Original terminal BODY observation, never a synthetic live envelope, delivery,
// generation or current-world publication permission. Absence is unknown.
struct zone_reset_item_retained_terminal
{
	bool present = false;
	critical_command original{};
	zone_reset_item_recovery_context context;
	std::vector<uint8_t> canonical;
};

class zone_reset_room_publication_owner;
class zone_reset_item_terminal_sql_owner final
{
    private:
	friend class zone_reset_room_publication_owner;
	// Actual world owner supplies the coordinator-pinned terminal phase2 carrier
	// after genuine physical ACK, before journal retirement. Borrow only its
	// reconnect-disabled IN_TRANS session; no commit, retry or generated ACK.
	static int retain_locked(MYSQL *, const critical_native_recovery_envelope &) noexcept;
	// Authenticate original successful root/TIR before locking terminal BODY.
	// No original origin => ENODATA. NULL terminal => successful present=false.
	// All errors preserve output; original admission rows remain unmodified.
	static int load_locked(MYSQL *, const critical_operation_id &,
			       zone_reset_item_retained_terminal *) noexcept;
};

#endif
