// migrate_common.c
// shared utility functions for pfile migration tool

#include "migrate_common.h"

// global vars
P_Guild guild_list = NULL;
char buf[MAX_STRING_LENGTH];
static unsigned long long g_obj_uid_counter = 1;

// format seconds into human readable time
static void format_time(double seconds, char *buf, size_t buflen) {
    if (seconds < 60) {
        snprintf(buf, buflen, "%ds", (int)seconds);
    } else if (seconds < 3600) {
        int mins = (int)(seconds / 60);
        int secs = (int)seconds % 60;
        snprintf(buf, buflen, "%dm%02ds", mins, secs);
    } else {
        int hours = (int)(seconds / 3600);
        int mins = ((int)seconds % 3600) / 60;
        int secs = (int)seconds % 60;
        snprintf(buf, buflen, "%dh%02dm%02ds", hours, mins, secs);
    }
}

void progress_init(struct progress_bar *pb, int total, const char *prefix) {
    pb->total = total;
    pb->current = 0;
    pb->prefix = prefix;
    gettimeofday(&pb->start_time, NULL);
    progress_update(pb, 0);
}

void progress_update(struct progress_bar *pb, int current) {
    pb->current = current;

    struct timeval now;
    gettimeofday(&now, NULL);
    double elapsed = (now.tv_sec - pb->start_time.tv_sec) +
                     (now.tv_usec - pb->start_time.tv_usec) / 1000000.0;

    int width = 30;
    int filled = (pb->total > 0) ? (current * width) / pb->total : 0;
    int percent = (pb->total > 0) ? (current * 100) / pb->total : 0;

    char elapsed_str[32], eta_str[32];
    format_time(elapsed, elapsed_str, sizeof(elapsed_str));

    if (current > 0 && current < pb->total) {
        double rate = current / elapsed;
        double remaining = (pb->total - current) / rate;
        format_time(remaining, eta_str, sizeof(eta_str));
    } else if (current >= pb->total) {
        snprintf(eta_str, sizeof(eta_str), "0s");
    } else {
        snprintf(eta_str, sizeof(eta_str), "--");
    }

    printf("\r%-12s [", pb->prefix);
    for (int i = 0; i < width; i++) {
        if (i < filled) printf("=");
        else printf("-");
    }
    printf("] %3d%% %d/%d  %s<%s", percent, current, pb->total, elapsed_str, eta_str);
    fflush(stdout);
}

void progress_finish(struct progress_bar *pb) {
    progress_update(pb, pb->total);
    printf("\n");
}

// minimal stubs for functions we don't need
void logit(int type, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[debug] ");
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// fread_string from db.c - reads until ~
char *fread_string(FILE *fl) {
    static char buf[MAX_STRING_LENGTH];
    char *ptr = buf;
    char c;

    do {
        c = getc(fl);
    } while (isspace(c));

    if (c == '~') {
        return str_dup("");
    }

    while (c != '~' && !feof(fl)) {
        if (c == '\n' || c == '\r') {
            *ptr++ = ' ';
        } else {
            *ptr++ = c;
        }
        c = getc(fl);
    }
    *ptr = '\0';

    // trim trailing space
    while (ptr > buf && isspace(*(ptr-1))) {
        *--ptr = '\0';
    }

    return str_dup(buf);
}

void free_mig_obj(struct mig_obj *obj) {
    if (!obj) return;
    if (obj->name) free(obj->name);
    if (obj->short_descr) free(obj->short_descr);
    if (obj->description) free(obj->description);
    if (obj->action_descr) free(obj->action_descr);
    if (obj->spellbook_bits) free(obj->spellbook_bits);
    free_mig_obj(obj->contains);
    free_mig_obj(obj->next);
    free(obj);
}

// convert binary spell bitfield to json array string like "[101,203,456]"
static char *mig_spellbook_to_json(const char *bits, int size) {
    if (!bits || size <= 0) return NULL;

    char *buf = (char *)malloc(MIG_MAX_SKILLS * 6);
    if (!buf) return NULL;

    char *p = buf;
    *p++ = '[';

    int first = 1;
    for (int i = 0; i < MIG_MAX_SKILLS && (i / 8) < size; i++) {
        if (bits[i / 8] & (1 << (i % 8))) {
            if (!first) *p++ = ',';
            p += sprintf(p, "%d", i);
            first = 0;
        }
    }
    *p++ = ']';
    *p = '\0';

    return buf;
}

void free_mig_affect(struct mig_affect *af) {
    while (af) {
        struct mig_affect *next = af->next;
        if (af->wear_off_char) free(af->wear_off_char);
        if (af->wear_off_room) free(af->wear_off_room);
        free(af);
        af = next;
    }
}

void free_mig_player(struct mig_player *p) {
    if (!p) return;
    if (p->short_descr) free(p->short_descr);
    if (p->long_descr) free(p->long_descr);
    if (p->description) free(p->description);
    if (p->title) free(p->title);
    if (p->poof_in) free(p->poof_in);
    if (p->poof_out) free(p->poof_out);
    if (p->poof_in_sound) free(p->poof_in_sound);
    if (p->poof_out_sound) free(p->poof_out_sound);
    if (p->granted_cmds) free(p->granted_cmds);
    free_mig_affect(p->affects);
    for (int i = 0; i < MIG_MAX_WEAR; i++)
        free_mig_obj(p->equipment[i]);
    free_mig_obj(p->inventory);
    free(p);
}

// helper: format nullable string for sql query
static void format_sql_str(char *out, size_t sz, const char *escaped) {
    if (escaped)
        snprintf(out, sz, "'%s'", escaped);
    else
        strcpy(out, "NULL");
}

/// helper: format value - returns "NULL" if not set, otherwise the int
static void format_value(char *buf, size_t sz, struct mig_obj *obj, int idx) {
    if (obj->value_set & (1 << idx))
        snprintf(buf, sz, "%d", obj->value[idx]);
    else
        strcpy(buf, "NULL");
}

// helper: format extra_flags - returns "NULL" if not set, otherwise the value
static void format_extra_flags(char *buf, size_t sz, struct mig_obj *obj) {
    if (obj->extra_flags_set)
        snprintf(buf, sz, "%lu", obj->extra_flags);
    else
        strcpy(buf, "NULL");
}

// helper: format wear_flags - returns "NULL" if not set, otherwise the value
static void format_wear_flags(char *buf, size_t sz, struct mig_obj *obj) {
    if (obj->wear_flags_set)
        snprintf(buf, sz, "%d", obj->wear_flags);
    else
        strcpy(buf, "NULL");
}

// thread-local flag for item saves (set by parallel workers)
__thread int item_use_thread_db = 0;

// wrapper for escape based on thread mode
static char *item_escape(const char *str) {
    return item_use_thread_db ? tsql_escape_string(str) : sql_escape_string(str);
}

static bool item_qry(const char *format, ...) {
    char query[65536];
    va_list args;
    va_start(args, format);
    vsnprintf(query, sizeof(query), format, args);
    va_end(args);
    return item_use_thread_db ? tqry("%s", query) : qry("%s", query);
}

static MYSQL_RES *item_db_query(const char *format, ...) {
    char query[65536];
    va_list args;
    va_start(args, format);
    vsnprintf(query, sizeof(query), format, args);
    va_end(args);
    return item_use_thread_db ? tdb_query("%s", query) : db_query("%s", query);
}

// generic item save - handles player_items, locker_items, corpse_items, saved_items
int save_item_to_db(struct mig_obj *obj, const char *table,
                    const char *owner_col, int owner_id,
                    int container_id, int equip_slot) {
    if (!obj) return 0;

    // escape strings
    char *esc_name = obj->name ? item_escape(obj->name) : NULL;
    char *esc_short = obj->short_descr ? item_escape(obj->short_descr) : NULL;
    char *esc_desc = obj->description ? item_escape(obj->description) : NULL;
    char *esc_action = obj->action_descr ? item_escape(obj->action_descr) : NULL;

    // format for sql
    char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
    format_sql_str(name_str, sizeof(name_str), esc_name);
    format_sql_str(short_str, sizeof(short_str), esc_short);
    format_sql_str(desc_str, sizeof(desc_str), esc_desc);
    format_sql_str(action_str, sizeof(action_str), esc_action);

    char container_str[32];
    if (container_id > 0)
        snprintf(container_str, sizeof(container_str), "%d", container_id);
    else
        strcpy(container_str, "NULL");

    // format values
    char v0[16], v1[16], v2[16], v3[16], v4[16], v5[16], v6[16], v7[16];
    format_value(v0, sizeof(v0), obj, 0);
    format_value(v1, sizeof(v1), obj, 1);
    format_value(v2, sizeof(v2), obj, 2);
    format_value(v3, sizeof(v3), obj, 3);
    format_value(v4, sizeof(v4), obj, 4);
    format_value(v5, sizeof(v5), obj, 5);
    format_value(v6, sizeof(v6), obj, 6);
    format_value(v7, sizeof(v7), obj, 7);

    // format weight/cost/timer (NULL if not modified from prototype)
    char weight_str[16], cost_str[16], timer_str[24];
    if (obj->weight_set) snprintf(weight_str, sizeof(weight_str), "%d", obj->weight); else strcpy(weight_str, "NULL");
    if (obj->cost_set) snprintf(cost_str, sizeof(cost_str), "%d", obj->cost); else strcpy(cost_str, "NULL");
    if (obj->timer_set) snprintf(timer_str, sizeof(timer_str), "%ld", obj->timer); else strcpy(timer_str, "NULL");

    // format extra_flags (NULL if not modified from prototype)
    char extra_str[32];
    format_extra_flags(extra_str, sizeof(extra_str), obj);

    // format wear_flags (NULL if not modified from prototype)
    char wear_str[32];
    format_wear_flags(wear_str, sizeof(wear_str), obj);

    // format item_type (NULL if not modified from prototype)
    char type_str[16];
    if (obj->item_type_set)
        snprintf(type_str, sizeof(type_str), "%d", obj->item_type);
    else
        strcpy(type_str, "NULL");

    // format bitvectors (NULL if not set)
    char bv1[32], bv2[32], bv3[32], bv4[32], bv5[32];
    if (obj->bitvector_set & 1) snprintf(bv1, sizeof(bv1), "%lu", obj->bitvector1); else strcpy(bv1, "NULL");
    if (obj->bitvector_set & 2) snprintf(bv2, sizeof(bv2), "%lu", obj->bitvector2); else strcpy(bv2, "NULL");
    if (obj->bitvector_set & 4) snprintf(bv3, sizeof(bv3), "%lu", obj->bitvector3); else strcpy(bv3, "NULL");
    if (obj->bitvector_set & 8) snprintf(bv4, sizeof(bv4), "%lu", obj->bitvector4); else strcpy(bv4, "NULL");
    if (obj->bitvector_set & 16) snprintf(bv5, sizeof(bv5), "%lu", obj->bitvector5); else strcpy(bv5, "NULL");

    // build query based on table type
    unsigned long long obj_uid = g_obj_uid_counter++;
    char query[8192];
    if (strcmp(table, "player_items") == 0) {
        // player_items has equip_slot and bitvectors
        snprintf(query, sizeof(query),
            "INSERT INTO %s (%s, vnum, equip_slot, container_id, quantity, "
            "weight, cost, timer, extra_flags, wear_flags, item_type, "
            "value0, value1, value2, value3, value4, value5, value6, value7, "
            "name, short_descr, description, action_descr, "
            "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, obj_uid) VALUES ("
            "%d, %d, %d, %s, 1, %s, %s, %s, %s, %s, %s, "
            "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, "
            "%s, %s, %s, %s, %s, %llu)",
            table, owner_col,
            owner_id, obj->vnum, equip_slot, container_str,
            weight_str, cost_str, timer_str, extra_str, wear_str, type_str,
            v0, v1, v2, v3, v4, v5, v6, v7,
            name_str, short_str, desc_str, action_str,
            bv1, bv2, bv3, bv4, bv5, obj_uid);
    } else if (equip_slot >= 0) {
        // shopkeeper_items - has equip_slot but no bitvectors
        snprintf(query, sizeof(query),
            "INSERT INTO %s (%s, vnum, equip_slot, container_id, quantity, "
            "weight, cost, timer, extra_flags, wear_flags, item_type, "
            "value0, value1, value2, value3, value4, value5, value6, value7, "
            "name, short_descr, description, action_descr, obj_uid) VALUES ("
            "%d, %d, %d, %s, 1, %s, %s, %s, %s, %s, %s, "
            "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %llu)",
            table, owner_col,
            owner_id, obj->vnum, equip_slot, container_str,
            weight_str, cost_str, timer_str, extra_str, wear_str, type_str,
            v0, v1, v2, v3, v4, v5, v6, v7,
            name_str, short_str, desc_str, action_str, obj_uid);
    } else {
        // locker_items, corpse_items, saved_items - no equip_slot, no bitvectors
        snprintf(query, sizeof(query),
            "INSERT INTO %s (%s, vnum, container_id, quantity, "
            "weight, cost, timer, extra_flags, wear_flags, item_type, "
            "value0, value1, value2, value3, value4, value5, value6, value7, "
            "name, short_descr, description, action_descr, obj_uid) VALUES ("
            "%d, %d, %s, 1, %s, %s, %s, %s, %s, %s, "
            "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %llu)",
            table, owner_col,
            owner_id, obj->vnum, container_str,
            weight_str, cost_str, timer_str, extra_str, wear_str, type_str,
            v0, v1, v2, v3, v4, v5, v6, v7,
            name_str, short_str, desc_str, action_str, obj_uid);
    }

    if (esc_name) free(esc_name);
    if (esc_short) free(esc_short);
    if (esc_desc) free(esc_desc);
    if (esc_action) free(esc_action);

    if (!item_qry("%s", query))
        return 0;

    MYSQL_RES *result = item_db_query("SELECT LAST_INSERT_ID()");
    if (!result) return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    int item_id = row ? atoi(row[0]) : 0;
    mysql_free_result(result);

    // save spellbook data to player_item_extra_descr (only for player_items table)
    if (obj->spellbook_bits && item_id > 0 && strcmp(table, "player_items") == 0) {
        char *json = mig_spellbook_to_json(obj->spellbook_bits, obj->spellbook_size);
        if (json) {
            char *esc_json = item_escape(json);
            item_qry("INSERT INTO player_item_extra_descr (item_id, keyword, description) "
                     "VALUES (%d, 'SPELLBOOK', '%s')", item_id, esc_json ? esc_json : json);
            if (esc_json) free(esc_json);
            free(json);
        }
    }

    // save item affects for player_items and locker_items
    if (obj->affected_set && item_id > 0) {
        const char *affects_table = NULL;
        if (strcmp(table, "player_items") == 0)
            affects_table = "player_item_affects";
        else if (strcmp(table, "locker_items") == 0)
            affects_table = "locker_item_affects";

        if (affects_table) {
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0) {
                    // skip duplicates
                    int is_dup = 0;
                    for (int j = 0; j < i; j++) {
                        if (obj->affected[j].location == obj->affected[i].location &&
                            obj->affected[j].modifier == obj->affected[i].modifier) {
                            is_dup = 1;
                            break;
                        }
                    }
                    if (is_dup)
                        continue;

                    item_qry("INSERT INTO %s (item_id, location, modifier) "
                             "VALUES (%d, %d, %d)",
                             affects_table, item_id, obj->affected[i].location, obj->affected[i].modifier);
                }
            }
        }
    }

    // save contained items recursively
    for (struct mig_obj *c = obj->contains; c; c = c->next) {
        save_item_to_db(c, table, owner_col, owner_id, item_id, -1);
    }

    return item_id;
}

// directory walking callback type
typedef int (*dir_file_callback)(const char *filepath, const char *filename, void *userdata);

// count files in player dirs matching filter
int count_player_dir_files(const char *base_path, const char *filter_ext, const char *filter_exclude) {
    int total = 0;
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "%s/%c", base_path, letter);
        DIR *dir = opendir(dirname);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            // filter by extension if specified
            if (filter_ext) {
                if (!strstr(entry->d_name, filter_ext)) continue;
            }

            // exclude patterns
            if (filter_exclude) {
                if (strstr(entry->d_name, ".bak")) continue;
                if (strstr(entry->d_name, ".backup")) continue;
                if (strstr(entry->d_name, ".old")) continue;
                if (strstr(entry->d_name, ".preconvert")) continue;
            }

            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);
            struct stat st;
            if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode)) continue;

            total++;
        }
        closedir(dir);
    }
    return total;
}

// walk player dirs and call callback for each file
int walk_player_dirs(const char *base_path, const char *filter_ext, const char *filter_exclude,
                     dir_file_callback callback, void *userdata) {
    int processed = 0;
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "%s/%c", base_path, letter);
        DIR *dir = opendir(dirname);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            // filter by extension if specified
            if (filter_ext) {
                if (!strstr(entry->d_name, filter_ext)) continue;
            }

            // exclude patterns
            if (filter_exclude) {
                if (strstr(entry->d_name, ".bak")) continue;
                if (strstr(entry->d_name, ".backup")) continue;
                if (strstr(entry->d_name, ".old")) continue;
                if (strstr(entry->d_name, ".preconvert")) continue;
            }

            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);
            struct stat st;
            if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode)) continue;

            callback(filepath, entry->d_name, userdata);
            processed++;
        }
        closedir(dir);
    }
    return processed;
}

// thread-local database connection
__thread MYSQL *thread_db = NULL;
int g_num_threads = 1;

// mutex to serialize connection setup
static pthread_mutex_t connect_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_thread_db(void) {
    if (thread_db) return;

    // serialize connections to avoid overwhelming mysql
    pthread_mutex_lock(&connect_mutex);

    mysql_thread_init();
    thread_db = mysql_init(NULL);
    if (!thread_db) {
        fprintf(stderr, "thread %ld: mysql_init failed\n", (long)pthread_self());
        pthread_mutex_unlock(&connect_mutex);
        return;
    }

    const char *host = getenv("DB_HOST");
    const char *user = getenv("DB_USER");
    const char *passwd = getenv("DB_PASSWD");
    const char *dbname = getenv("DB_NAME");
    int port = atoi(getenv("DB_PORT") ? getenv("DB_PORT") : "3306");

    // set connection timeout
    unsigned int timeout = 30;
    mysql_options(thread_db, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(thread_db, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(thread_db, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

    if (!mysql_real_connect(thread_db, host, user, passwd, dbname, port, NULL, 0)) {
        fprintf(stderr, "thread %ld: mysql connect failed: %s\n",
                (long)pthread_self(), mysql_error(thread_db));
        mysql_close(thread_db);
        thread_db = NULL;
        pthread_mutex_unlock(&connect_mutex);
        return;
    }

    mysql_set_character_set(thread_db, "utf8mb4");
    pthread_mutex_unlock(&connect_mutex);
}

MYSQL *get_thread_db(void) {
    if (!thread_db) init_thread_db();
    return thread_db;
}

void close_thread_db(void) {
    if (thread_db) {
        mysql_close(thread_db);
        thread_db = NULL;
        mysql_thread_end();
    }
}

// mutex for error output to avoid garbled messages
static pthread_mutex_t error_mutex = PTHREAD_MUTEX_INITIALIZER;

// reconnect thread-local db
static void reconnect_thread_db(void) {
    if (thread_db) {
        mysql_close(thread_db);
        thread_db = NULL;
    }
    init_thread_db();
}

// thread-safe query with result (with reconnect on failure)
MYSQL_RES *tdb_query(const char *format, ...) {
    MYSQL *db = get_thread_db();
    if (!db) {
        pthread_mutex_lock(&error_mutex);
        fprintf(stderr, "tdb_query: no db connection\n");
        pthread_mutex_unlock(&error_mutex);
        return NULL;
    }

    char query[65536];
    va_list args;
    va_start(args, format);
    vsnprintf(query, sizeof(query), format, args);
    va_end(args);

    if (mysql_query(db, query)) {
        unsigned int err = mysql_errno(db);
        // CR_SERVER_GONE_ERROR=2006, CR_SERVER_LOST=2013
        if (err == 2006 || err == 2013) {
            // try to reconnect once
            reconnect_thread_db();
            db = get_thread_db();
            if (db && mysql_query(db, query) == 0) {
                return mysql_store_result(db);
            }
        }
        pthread_mutex_lock(&error_mutex);
        fprintf(stderr, "sql error: %s\nquery: %.200s...\n",
                mysql_error(db), query);
        pthread_mutex_unlock(&error_mutex);
        return NULL;
    }

    return mysql_store_result(db);
}

// thread-safe query without result (with reconnect on failure)
bool tqry(const char *format, ...) {
    MYSQL *db = get_thread_db();
    if (!db) {
        pthread_mutex_lock(&error_mutex);
        fprintf(stderr, "tqry: no db connection\n");
        pthread_mutex_unlock(&error_mutex);
        return false;
    }

    char query[65536];
    va_list args;
    va_start(args, format);
    vsnprintf(query, sizeof(query), format, args);
    va_end(args);

    if (mysql_query(db, query)) {
        unsigned int err = mysql_errno(db);
        // CR_SERVER_GONE_ERROR=2006, CR_SERVER_LOST=2013
        if (err == 2006 || err == 2013) {
            // try to reconnect once
            reconnect_thread_db();
            db = get_thread_db();
            if (db && mysql_query(db, query) == 0) {
                return true;
            }
        }
        pthread_mutex_lock(&error_mutex);
        fprintf(stderr, "sql error: %s\nquery: %.200s...\n",
                mysql_error(db), query);
        pthread_mutex_unlock(&error_mutex);
        return false;
    }

    return true;
}

// thread-safe string escape
char *tsql_escape_string(const char *str) {
    if (!str) return NULL;

    MYSQL *db = get_thread_db();
    if (!db) return NULL;

    size_t len = strlen(str);
    char *escaped = (char *)malloc(len * 2 + 1);
    if (!escaped) return NULL;

    mysql_real_escape_string(db, escaped, str, len);
    return escaped;
}

// work queue implementation
void work_queue_init(struct work_queue *wq, char **files, int count, struct progress_bar *pb) {
    wq->files = files;
    wq->total = count;
    wq->next_index = 0;
    wq->completed = 0;
    wq->success = 0;
    wq->errors = 0;
    wq->pb = pb;
    pthread_mutex_init(&wq->mutex, NULL);
}

char *work_queue_get(struct work_queue *wq) {
    char *file = NULL;
    pthread_mutex_lock(&wq->mutex);
    if (wq->next_index < wq->total) {
        file = wq->files[wq->next_index++];
    }
    pthread_mutex_unlock(&wq->mutex);
    return file;
}

void work_queue_done(struct work_queue *wq, int success) {
    pthread_mutex_lock(&wq->mutex);
    wq->completed++;
    if (success)
        wq->success++;
    else
        wq->errors++;
    if (wq->pb)
        progress_update(wq->pb, wq->completed);
    pthread_mutex_unlock(&wq->mutex);
}

void work_queue_destroy(struct work_queue *wq) {
    pthread_mutex_destroy(&wq->mutex);
}
