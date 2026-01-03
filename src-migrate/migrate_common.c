// migrate_common.c
// shared utility functions for pfile migration tool

#include "migrate_common.h"

// global vars
P_Guild guild_list = NULL;
char buf[MAX_STRING_LENGTH];

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
    free_mig_obj(obj->contains);
    free_mig_obj(obj->next);
    free(obj);
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

// helper: format value - returns "NULL" if not set, otherwise the int
static void format_value(char *buf, size_t sz, struct mig_obj *obj, int idx) {
    if (obj->value_set & (1 << idx))
        snprintf(buf, sz, "%d", obj->value[idx]);
    else
        strcpy(buf, "NULL");
}

// generic item save - handles player_items, locker_items, corpse_items, saved_items
int save_item_to_db(struct mig_obj *obj, const char *table,
                    const char *owner_col, int owner_id,
                    int container_id, int equip_slot) {
    if (!obj) return 0;

    // escape strings
    char *esc_name = obj->name ? sql_escape_string(obj->name) : NULL;
    char *esc_short = obj->short_descr ? sql_escape_string(obj->short_descr) : NULL;
    char *esc_desc = obj->description ? sql_escape_string(obj->description) : NULL;
    char *esc_action = obj->action_descr ? sql_escape_string(obj->action_descr) : NULL;

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

    // build query based on table type
    char query[8192];
    if (equip_slot >= 0) {
        // player_items has equip_slot
        snprintf(query, sizeof(query),
            "INSERT INTO %s (%s, vnum, equip_slot, container_id, quantity, "
            "weight, cost, timer, extra_flags, "
            "value0, value1, value2, value3, value4, value5, value6, value7, "
            "name, short_descr, description, action_descr) VALUES ("
            "%d, %d, %d, %s, 1, %d, %d, %ld, %lu, "
            "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)",
            table, owner_col,
            owner_id, obj->vnum, equip_slot, container_str,
            obj->weight, obj->cost, obj->timer, obj->extra_flags,
            v0, v1, v2, v3, v4, v5, v6, v7,
            name_str, short_str, desc_str, action_str);
    } else {
        // locker_items, corpse_items - no equip_slot
        snprintf(query, sizeof(query),
            "INSERT INTO %s (%s, vnum, container_id, quantity, "
            "weight, cost, timer, extra_flags, "
            "value0, value1, value2, value3, value4, value5, value6, value7, "
            "name, short_descr, description, action_descr) VALUES ("
            "%d, %d, %s, 1, %d, %d, %ld, %lu, "
            "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)",
            table, owner_col,
            owner_id, obj->vnum, container_str,
            obj->weight, obj->cost, obj->timer, obj->extra_flags,
            v0, v1, v2, v3, v4, v5, v6, v7,
            name_str, short_str, desc_str, action_str);
    }

    if (esc_name) free(esc_name);
    if (esc_short) free(esc_short);
    if (esc_desc) free(esc_desc);
    if (esc_action) free(esc_action);

    if (!qry("%s", query))
        return 0;

    MYSQL_RES *result = db_query("SELECT LAST_INSERT_ID()");
    if (!result) return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    int item_id = row ? atoi(row[0]) : 0;
    mysql_free_result(result);

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
