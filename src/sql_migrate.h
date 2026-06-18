#ifndef __SQL_MIGRATE_H__
#define __SQL_MIGRATE_H__

/*
 * sql_migrate.h — Schema migration auto-runner
 *
 * When MIGRATION_AUTO_RUNNER is defined (via Makefile -D flag), the real
 * implementation in sql_migrate.c is compiled and sql_run_migrations()
 * scans versioned schema_migration_v*.sql files in the migrations/
 * directory, executes unapplied ones in numeric order, and records them
 * in a schema_migrations table.
 *
 * When MIGRATION_AUTO_RUNNER is NOT defined, sql_run_migrations() is a
 * no-op stub (returns 0).  Shell scripts (entrypoint.sh, cycle_mud.sh)
 * retain their manual migration lines as the fallback path.
 *
 * Usage in Makefile:
 *   CFLAGS += -DMIGRATION_AUTO_RUNNER    # enable auto-runner
 *   # omit to disable
 */

#ifndef __NO_MYSQL__
#include <mysql.h>
#endif

#ifdef MIGRATION_AUTO_RUNNER

/* Run all unapplied migrations from the given directory.
 * Creates schema_migrations tracking table if absent.
 * Returns 0 on success, -1 on failure (MUD should abort boot).
 *
 * db               — connected MySQL handle
 * migrations_dir   — path to directory containing .sql files */
int sql_run_migrations(MYSQL *db, const char *migrations_dir);

#else

/* Stub when auto-runner is disabled — always succeeds.
 * Uses void* to avoid needing <mysql.h> when __NO_MYSQL__ is defined. */
static inline int sql_run_migrations(void *db, const char *migrations_dir)
{
    (void)db;
    (void)migrations_dir;
    return 0;
}

#endif /* MIGRATION_AUTO_RUNNER */

#endif /* __SQL_MIGRATE_H__ */
