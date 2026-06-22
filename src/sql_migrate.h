#ifndef __SQL_MIGRATE_H__
#define __SQL_MIGRATE_H__

/*
 * Schema migrations are handled externally by shell scripts / release
 * tooling. The in-process auto-runner was removed.
 */

#ifndef __NO_MYSQL__
#include <mysql.h>
#endif

static inline int sql_run_migrations(void *db, const char *migrations_dir)
{
    (void)db;
    (void)migrations_dir;
    return 0;
}

#endif /* __SQL_MIGRATE_H__ */
