// migrate_lockers.c
// locker migration for pfile migration tool

#include "migrate_common.h"

// save locker item to database, returns item_id
int save_locker_item(int locker_id, struct mig_obj *obj, int container_id) {
    if (!obj || locker_id <= 0) return 0;

    char *esc_name = obj->name ? sql_escape_string(obj->name) : NULL;
    char *esc_short = obj->short_descr ? sql_escape_string(obj->short_descr) : NULL;
    char *esc_desc = obj->description ? sql_escape_string(obj->description) : NULL;
    char *esc_action = obj->action_descr ? sql_escape_string(obj->action_descr) : NULL;

    char container_str[32];
    if (container_id > 0)
        snprintf(container_str, sizeof(container_str), "%d", container_id);
    else
        strcpy(container_str, "NULL");

    char name_str[1024], short_str[1024], desc_str[2048], action_str[2048];
    if (esc_name) snprintf(name_str, sizeof(name_str), "'%s'", esc_name);
    else strcpy(name_str, "NULL");
    if (esc_short) snprintf(short_str, sizeof(short_str), "'%s'", esc_short);
    else strcpy(short_str, "NULL");
    if (esc_desc) snprintf(desc_str, sizeof(desc_str), "'%s'", esc_desc);
    else strcpy(desc_str, "NULL");
    if (esc_action) snprintf(action_str, sizeof(action_str), "'%s'", esc_action);
    else strcpy(action_str, "NULL");

    char query[8192];
    snprintf(query, sizeof(query),
        "INSERT INTO locker_items ("
        "locker_id, vnum, container_id, quantity, "
        "weight, cost, timer, extra_flags, "
        "value0, value1, value2, value3, value4, value5, value6, value7, "
        "name, short_descr, description, action_descr"
        ") VALUES ("
        "%d, %d, %s, 1, "
        "%d, %d, %ld, %lu, "
        "%d, %d, %d, %d, %d, %d, %d, %d, "
        "%s, %s, %s, %s"
        ")",
        locker_id, obj->vnum, container_str,
        obj->weight, obj->cost, obj->timer, obj->extra_flags,
        obj->value[0], obj->value[1], obj->value[2], obj->value[3],
        obj->value[4], obj->value[5], obj->value[6], obj->value[7],
        name_str, short_str, desc_str, action_str);

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
        save_locker_item(locker_id, c, item_id);
    }

    return item_id;
}

// migrate a single locker file
static int migrate_locker_file(const char *filepath, const char *locker_name) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return 0;

    char buffer[250000];
    int size = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);

    if (size < 50) return 0;

    char *bufptr = buffer;

    // check pfile header
    int save_vers = MIG_GET_BYTE(bufptr);
    if (save_vers != SAV_SAVEVERS) {
        printf("  wrong locker version: %d\n", save_vers);
        return 0;
    }

    int ss = MIG_GET_BYTE(bufptr);
    int is = MIG_GET_BYTE(bufptr);
    int ls = MIG_GET_BYTE(bufptr);
    if (ss != 2 || is != 4 || ls != 8) {
        printf("  wrong locker machine format: %d/%d/%d\n", ss, is, ls);
        return 0;
    }

    MIG_GET_BYTE(bufptr); // rent type

    // read offsets
    mig_getInt(&bufptr); // skill_off
    mig_getInt(&bufptr); // witness_off
    mig_getInt(&bufptr); // affect_off
    int item_off = mig_getInt(&bufptr);
    mig_getInt(&bufptr); // size_off
    mig_getInt(&bufptr); // act3/surname
    mig_getInt(&bufptr); // room
    mig_getLong(&bufptr); // save time

    // validate item offset
    if (item_off < 0 || item_off >= size) {
        printf("  invalid locker item_off: %d (size: %d)\n", item_off, size);
        return 0;
    }

    // parse status section to get race/racewar
    int stat_vers = MIG_GET_BYTE(bufptr);
    if (stat_vers > SAV_STATVERS) {
        printf("  wrong locker stat version: %d\n", stat_vers);
        return 0;
    }

    // read some fields to get race/racewar
    char *tmp_str;
    tmp_str = mig_getString(&bufptr); if (tmp_str) free(tmp_str); // name
    if (stat_vers >= 32) {
        tmp_str = mig_getString(&bufptr); if (tmp_str) free(tmp_str);
    } else {
        tmp_str = mig_getString(&bufptr); if (tmp_str) free(tmp_str);
        tmp_str = mig_getString(&bufptr); if (tmp_str) free(tmp_str);
        tmp_str = mig_getString(&bufptr); if (tmp_str) free(tmp_str);
    }
    tmp_str = mig_getString(&bufptr); if (tmp_str) free(tmp_str); // password
    tmp_str = mig_getString(&bufptr); if (tmp_str) free(tmp_str); // title

    // m_class
    if (stat_vers < 33) {
        mig_getInt(&bufptr);
        mig_getInt(&bufptr);
    } else {
        mig_getInt(&bufptr);
    }
    if (stat_vers > 36) mig_getInt(&bufptr); // secondary_class
    if (stat_vers > 38) MIG_GET_BYTE(bufptr); // spec

    int race = MIG_GET_BYTE(bufptr);
    int racewar = MIG_GET_BYTE(bufptr);

    // determine owner from locker name
    int owner_pid = 0;
    int owner_assoc_id = 0;

    // guild locker: "guild.X.locker"
    if (strncasecmp(locker_name, "guild.", 6) == 0) {
        const char *p = locker_name + 6;
        owner_assoc_id = atoi(p);
    } else {
        // player locker: "playername.locker"
        char pname[128];
        strncpy(pname, locker_name, sizeof(pname) - 1);
        pname[sizeof(pname) - 1] = '\0';
        char *dot = strstr(pname, ".locker");
        if (dot) *dot = '\0';

        char *esc = sql_escape_string(pname);
        if (esc) {
            MYSQL_RES *res = db_query("SELECT pid FROM player_data WHERE LOWER(name) = LOWER('%s')", esc);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row && row[0]) owner_pid = atoi(row[0]);
                mysql_free_result(res);
            }
            free(esc);
        }
    }

    // delete existing locker
    char *esc_name = sql_escape_string(locker_name);
    if (!esc_name) return 0;
    qry("DELETE FROM lockers WHERE locker_name = '%s'", esc_name);

    // insert locker record
    char owner_pid_str[32], owner_assoc_str[32];
    if (owner_pid > 0)
        snprintf(owner_pid_str, sizeof(owner_pid_str), "%d", owner_pid);
    else
        strcpy(owner_pid_str, "NULL");
    if (owner_assoc_id > 0)
        snprintf(owner_assoc_str, sizeof(owner_assoc_str), "%d", owner_assoc_id);
    else
        strcpy(owner_assoc_str, "NULL");

    if (!qry("INSERT INTO lockers (locker_name, owner_pid, owner_assoc_id, racewar, race) "
             "VALUES ('%s', %s, %s, %d, %d)",
             esc_name, owner_pid_str, owner_assoc_str, racewar, race)) {
        free(esc_name);
        return 0;
    }
    free(esc_name);

    MYSQL_RES *result = db_query("SELECT LAST_INSERT_ID()");
    if (!result) return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    int locker_id = row ? atoi(row[0]) : 0;
    mysql_free_result(result);

    if (locker_id <= 0) return 0;

    // parse and save items
    bufptr = buffer + item_off;
    struct mig_obj *items = parse_locker_items(&bufptr);
    int item_count = 0;
    for (struct mig_obj *obj = items; obj; obj = obj->next) {
        save_locker_item(locker_id, obj, 0);
        item_count++;
    }

    free_mig_obj(items);

    return item_count;
}

// count locker files for progress bar
static int count_locker_files(void) {
    int total = 0;
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Players/%c", letter);
        DIR *dir = opendir(dirname);
        if (!dir) continue;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (!strstr(entry->d_name, ".locker")) continue;
            total++;
        }
        closedir(dir);
    }
    return total;
}

// migrate all locker files
int migrate_lockers_from_files(void) {
    int count = 0;
    int errors = 0;
    int total_items = 0;
    int processed = 0;

    int total = count_locker_files();
    struct progress_bar pb;
    progress_init(&pb, total, "lockers");

    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Players/%c", letter);

        DIR *dir = opendir(dirname);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (!strstr(entry->d_name, ".locker")) continue;

            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);

            int items = migrate_locker_file(filepath, entry->d_name);
            if (items >= 0) {
                count++;
                total_items += items;
            } else {
                errors++;
            }

            processed++;
            progress_update(&pb, processed);
        }
        closedir(dir);
    }

    progress_finish(&pb);
    printf("lockers: %d migrated, %d items, %d errors\n", count, total_items, errors);
    return count;
}
