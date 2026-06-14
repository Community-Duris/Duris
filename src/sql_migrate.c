/************************************************************************
 * sql_migrate.c — Schema migration auto-runner
 *
 * Called from initialize_mysql() during MUD boot when
 * MIGRATION_AUTO_RUNNER is defined.
 *
 * Scans the migrations/ directory for .sql files, executes
 * unapplied ones in alphabetical order, and records them in
 * a schema_migrations tracking table.
 *
 * On first run against an existing database (schema_migrations is
 * empty but player_data exists), all migration files are recorded
 * as "applied" without executing SQL — safe for existing installs.
 ************************************************************************/

#ifdef MIGRATION_AUTO_RUNNER

#include "sql_migrate.h"
#include "sql.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static int ends_with(const char *s, const char *suffix)
{
    size_t slen = strlen(s);
    size_t xlen = strlen(suffix);
    return slen >= xlen && strcmp(s + slen - xlen, suffix) == 0;
}

/* Compare function for qsort — alphabetical */
static int cmp_alpha(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

/* Check if a migration has already been applied */
static int migration_applied(MYSQL *db, const char *filename)
{
    char escaped[512];
    mysql_real_escape_string(db, escaped, filename, strlen(filename));

    char query[600];
    snprintf(query, sizeof(query),
             "SELECT 1 FROM schema_migrations WHERE version='%s'", escaped);

    if (mysql_real_query(db, query, strlen(query)) != 0)
        return 0;

    MYSQL_RES *res = mysql_store_result(db);
    if (!res)
        return 0;

    int applied = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return applied;
}

/* Check if this looks like an existing database (not a fresh install) */
static int is_existing_database(MYSQL *db)
{
    const char *query =
        "SELECT 1 FROM information_schema.tables "
        "WHERE table_schema = DATABASE() "
        "AND table_name = 'player_data'";

    if (mysql_real_query(db, query, strlen(query)) != 0)
        return 0;

    MYSQL_RES *res = mysql_store_result(db);
    if (!res)
        return 0;

    int exists = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exists;
}

/* ------------------------------------------------------------------ */
/*  Idempotency Gate                                                   */
/* ------------------------------------------------------------------ */

/*
 * Scan a migration file's SQL content for unsafe (non-idempotent)
 * patterns.  Returns 0 if the file passes the gate, -1 if unsafe
 * patterns are found (with a diagnostic to stderr).
 *
 * Unsafe patterns (rejected):
 *   - CREATE TABLE without IF NOT EXISTS  (will fail on re-run)
 *   - ALTER TABLE without information_schema guard in the file
 *   - CREATE INDEX without IF NOT EXISTS and without info_schema guard
 *
 * Safe patterns (allowed through):
 *   - CREATE TABLE IF NOT EXISTS
 *   - ALTER TABLE guarded by information_schema check (anywhere in file)
 *   - CREATE INDEX IF NOT EXISTS or guarded by information_schema
 *   - DROP TABLE IF EXISTS, DROP PROCEDURE IF EXISTS
 *   - CREATE OR REPLACE VIEW
 *   - INSERT IGNORE, REPLACE INTO
 *   - UPDATE, DELETE (idempotent for dedup/cleanup)
 *   - SET @var, PREPARE, EXECUTE, DEALLOCATE
 *   - DELIMITER, BEGIN, END, CALL, IF/THEN/ELSEIF (stored procs)
 */
static int validate_migration_idempotency(const char *sql_text,
                                          const char *filename)
{
    /* Does the file contain information_schema queries?  ALTER TABLE
     * and CREATE INDEX are only allowed when the file also queries
     * information_schema to guard them. */
    int has_info_schema = (strstr(sql_text, "information_schema") != NULL);

    /* Work on a mutable copy for line-by-line scanning */
    char *buf = strdup(sql_text);
    if (!buf) return -1;

    char *line    = buf;
    int   lineno  = 0;
    int   in_block_comment = 0;

    while (1) {
        /* Extract next line */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';   /* terminate this line in-place */

        lineno++;

        /* Skip leading whitespace */
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r') p++;

        /* Track multiline block comments */
        if (in_block_comment) {
            if (strstr(p, "*/"))
                in_block_comment = 0;
            goto next;
        }

        /* Skip SQL single-line comments and empty lines */
        if (*p == '\0' || *p == '-' || *p == '#')
            goto next;

        /* Enter block comment */
        if (*p == '/' && *(p+1) == '*') {
            in_block_comment = 1;
            if (strstr(p, "*/")) in_block_comment = 0; /* single-line */
            goto next;
        }

        /* ── Gate 1: CREATE TABLE without IF NOT EXISTS ─────────── */
        if (strstr(p, "CREATE TABLE") && !strstr(p, "IF NOT EXISTS")) {
            fprintf(stderr,
                "migrate: %s line %d: unsafe CREATE TABLE without "
                "IF NOT EXISTS\n"
                "  Fix: CREATE TABLE IF NOT EXISTS ...\n",
                filename, lineno);
            free(buf);
            return -1;
        }

        /* ── Gate 2: ALTER TABLE without information_schema guard ─ */
        if (strstr(p, "ALTER TABLE") && !has_info_schema) {
            fprintf(stderr,
                "migrate: %s line %d: unsafe ALTER TABLE without "
                "information_schema guard\n"
                "  Fix: wrap with SET @col_exists = (SELECT COUNT(*) "
                "FROM information_schema.columns ...)\n",
                filename, lineno);
            free(buf);
            return -1;
        }

        /* ── Gate 3: CREATE [UNIQUE] INDEX without guard ────────── */
        if ((strstr(p, "CREATE INDEX") || strstr(p, "CREATE UNIQUE INDEX")) &&
            !strstr(p, "IF NOT EXISTS") &&
            !has_info_schema) {
            fprintf(stderr,
                "migrate: %s line %d: unsafe CREATE INDEX without "
                "guard\n"
                "  Fix: CREATE INDEX IF NOT EXISTS, "
                "or wrap with information_schema check\n",
                filename, lineno);
            free(buf);
            return -1;
        }

next:
        if (!nl) break;          /* end of string */
        line = nl + 1;           /* advance to next line */
    }

    free(buf);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Execute a migration file's SQL                                     */
/* ------------------------------------------------------------------ */

static int execute_migration_file(MYSQL *db, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "migrate: cannot open %s\n", path);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *sql_text = (char *)malloc(sz + 1);
    if (!sql_text) {
        fclose(f);
        return -1;
    }

    size_t rd = fread(sql_text, 1, sz, f);
    sql_text[rd] = '\0';
    fclose(f);

    /* ── Idempotency gate: reject unguarded migration files ────── */
    if (validate_migration_idempotency(sql_text, path) != 0) {
        free(sql_text);
        return -1;
    }

    /* Execute — DDL causes implicit commits in MySQL, so the
     * BEGIN/COMMIT wrapper is best-effort.  Idempotent migrations
     * (CREATE IF NOT EXISTS, INSERT IGNORE, etc.) are the real
     * safety net for DDL; the transaction protects DML-only
     * migrations like cleanup scripts. */
    mysql_real_query(db, "BEGIN", 5);

    if (mysql_real_query(db, sql_text, rd) != 0) {
        fprintf(stderr, "migrate: SQL error in %s: %s\n",
                path, mysql_error(db));
        mysql_real_query(db, "ROLLBACK", 7);
        free(sql_text);
        return -1;
    }

    mysql_real_query(db, "COMMIT", 6);

    /* Drain multi-statement results */
    int status = 0;
    do {
        MYSQL_RES *result = mysql_store_result(db);
        if (result) mysql_free_result(result);
        if ((status = mysql_next_result(db)) > 0) {
            fprintf(stderr, "migrate: error draining results: %s\n",
                    mysql_error(db));
            break;
        }
    } while (status == 0);

    free(sql_text);
    return 0;
}

/* Record a migration as applied */
static int record_migration(MYSQL *db, const char *filename)
{
    char escaped[512];
    mysql_real_escape_string(db, escaped, filename, strlen(filename));

    char query[600];
    snprintf(query, sizeof(query),
             "INSERT INTO schema_migrations (version) VALUES ('%s')", escaped);

    if (mysql_real_query(db, query, strlen(query)) != 0) {
        fprintf(stderr, "migrate: failed to record %s: %s\n",
                filename, mysql_error(db));
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

int sql_run_migrations(MYSQL *db, const char *migrations_dir)
{
    if (!db || !migrations_dir)
        return -1;

    printf("=== Schema Migration Auto-Runner ===\n");

    /* 1. Ensure tracking table exists */
    {
        const char *query =
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "version VARCHAR(255) PRIMARY KEY, "
            "applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";

        if (mysql_real_query(db, query, strlen(query)) != 0) {
        fprintf(stderr, "migrate: cannot create schema_migrations: %s\n",
                mysql_error(db));
        return -1;
    }

    /* 2. Scan migrations directory */
    DIR *dir = opendir(migrations_dir);
    if (!dir) {
        fprintf(stderr, "migrate: cannot open %s\n", migrations_dir);
        return -1;
    }

    char *files[256];
    int   n = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) && n < 256) {
        if (ends_with(entry->d_name, ".sql"))
            files[n++] = strdup(entry->d_name);
    }
    closedir(dir);

    if (n == 0) {
        printf("  No .sql files found in %s/\n", migrations_dir);
        return 0;
    }

    /* Sort alphabetically */
    qsort(files, n, sizeof(char *), cmp_alpha);

    /* 3. Bootstrap: if this is an existing database with no migration
     *    records, mark all files as applied without executing */
    int total_applied = 0;
    for (int i = 0; i < n; i++)
        total_applied += migration_applied(db, files[i]);

    if (total_applied == 0 && is_existing_database(db)) {
        printf("  Bootstrapping migration tracking for existing database...\n");
        for (int i = 0; i < n; i++) {
            if (record_migration(db, files[i]) != 0) {
                for (int j = 0; j < n; j++) free(files[j]);
                return -1;
            }
        }
        printf("  %d migrations recorded as 'applied' (pre-existing DB)\n", n);
        for (int i = 0; i < n; i++) free(files[i]);
        return 0;
    }

    /* 4. Run unapplied migrations */
    int applied = 0, failed = 0;
    for (int i = 0; i < n; i++) {
        if (migration_applied(db, files[i]))
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", migrations_dir, files[i]);

        printf("  Applying: %s ... ", files[i]);
        fflush(stdout);

        if (execute_migration_file(db, path) != 0) {
            printf("FAILED\n");
            failed++;
            break;
        }

        if (record_migration(db, files[i]) != 0) {
            printf("FAILED (record)\n");
            failed++;
            break;
        }

        printf("OK\n");
        applied++;
    }

    for (int i = 0; i < n; i++) free(files[i]);

    if (failed > 0) {
        fprintf(stderr,
                "migrate: %d applied, %d failed — MUD boot aborted\n",
                applied, failed);
        return -1;
    }

    printf("  %d applied, %d skipped (already applied)\n",
           applied, n - applied - total_applied);
    return 0;
}

#endif /* MIGRATION_AUTO_RUNNER */
