#ifndef DURIS_ECONOMIC_SQL_LIFECYCLE_LOCK_NAMES_H
#define DURIS_ECONOMIC_SQL_LIFECYCLE_LOCK_NAMES_H

// Native runtime/maintenance ownership and independent SQL probes must use
// exactly the same lock identity. Keep this contract free of SQL-client types.
inline constexpr const char ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME[] =
	"duris:economic_sql_boot_maintenance";

#endif
