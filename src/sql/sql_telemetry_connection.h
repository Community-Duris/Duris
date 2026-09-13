#ifndef DURIS_SQL_TELEMETRY_CONNECTION_H
#define DURIS_SQL_TELEMETRY_CONNECTION_H

/* Private connection factory; credentials and handles never enter telemetry values.
 * Uses the same target, TLS and session validation as the gameplay factory, but
 * requires an explicitly provisioned least-privilege ingest credential pair. */
#ifndef __NO_MYSQL__
#include <mysql.h>
MYSQL *sql_open_telemetry_connection(void);
#endif

#endif
