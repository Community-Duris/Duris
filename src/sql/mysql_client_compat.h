#ifndef DURIS_SQL_MYSQL_CLIENT_COMPAT_H
#define DURIS_SQL_MYSQL_CLIENT_COMPAT_H

#ifndef __NO_MYSQL__
#include <mysql.h>

/* MariaDB Connector/C provides a named accessor. Ubuntu's Oracle libmysqlclient
 * package does not ship one, but its installed public MYSQL/NET definitions
 * retain NET::fd specifically for external DBI integrations. Isolate that ABI
 * field here so the copyover contract has one auditable client boundary. */
static inline int sql_mysql_socket_descriptor(MYSQL *connection)
{
#if defined(MARIADB_BASE_VERSION) || defined(MARIADB_PACKAGE_VERSION)
	return static_cast<int>(mysql_get_socket(connection));
#else
	return static_cast<int>(connection->net.fd);
#endif
}
#endif

#endif
