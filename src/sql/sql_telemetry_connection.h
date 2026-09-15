#ifndef DURIS_SQL_TELEMETRY_CONNECTION_H
#define DURIS_SQL_TELEMETRY_CONNECTION_H

/* Private connection factory; credentials and handles never enter telemetry values.
 * Uses the same target, TLS and session validation as the gameplay factory, but
 * requires an explicitly provisioned least-privilege ingest credential pair. */
#ifndef __NO_MYSQL__
#include <mysql.h>
// MariaDB exposes its PVIO socket through an accessor. Oracle's public client
// structure exposes the connection descriptor directly instead.
inline int sql_telemetry_socket(MYSQL *connection)
{
#if defined(MARIADB_BASE_VERSION) || defined(MARIADB_PACKAGE_VERSION)
	return static_cast<int>(mysql_get_socket(connection));
#else
	return static_cast<int>(connection->net.fd);
#endif
}
MYSQL *sql_open_telemetry_connection(void);
#endif

#endif
