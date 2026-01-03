// migrate_misc.c
// corpses, saved items, recipes, spellbooks migration

#include "migrate_common.h"

// save a mig_obj to corpse_items table, returns item_id
static int save_corpse_item(int corpse_id, struct mig_obj *obj, int container_id) {
    if (!obj) return 0;

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
        "INSERT INTO corpse_items ("
        "corpse_id, vnum, container_id, quantity, "
        "weight, cost, timer, extra_flags, "
        "value0, value1, value2, value3, value4, value5, value6, value7, "
        "name, short_descr, description, action_descr"
        ") VALUES ("
        "%d, %d, %s, 1, "
        "%d, %d, %ld, %lu, "
        "%d, %d, %d, %d, %d, %d, %d, %d, "
        "%s, %s, %s, %s"
        ")",
        corpse_id, obj->vnum, container_str,
        obj->weight, obj->cost, obj->timer, obj->extra_flags,
        obj->value[0], obj->value[1], obj->value[2], obj->value[3],
        obj->value[4], obj->value[5], obj->value[6], obj->value[7],
        name_str, short_str, desc_str, action_str
    );

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

    // save contained objects
    for (struct mig_obj *c = obj->contains; c; c = c->next) {
        save_corpse_item(corpse_id, c, item_id);
    }

    return item_id;
}

// count corpse files
static int count_corpse_files(void) {
    int total = 0;
    DIR *dir = opendir("Players/Corpses");
    if (!dir) return 0;
    struct dirent *de;
    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.') continue;
        if (strstr(de->d_name, ".bak")) continue;
        if (strstr(de->d_name, ".backup")) continue;
        if (strstr(de->d_name, ".old")) continue;
        total++;
    }
    closedir(dir);
    return total;
}

int migrate_corpses_from_files(void) {
    int count = 0;
    int errors = 0;
    int items_total = 0;
    int processed = 0;
    char buffer[240000];

    int total = count_corpse_files();
    struct progress_bar pb;
    progress_init(&pb, total, "corpses");

    DIR *dir = opendir("Players/Corpses");
    if (!dir) {
        progress_finish(&pb);
        return 0;
    }

    struct dirent *de;
    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.') continue;
        if (strstr(de->d_name, ".bak")) continue;
        if (strstr(de->d_name, ".backup")) continue;
        if (strstr(de->d_name, ".old")) continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "Players/Corpses/%s", de->d_name);

        FILE *f = fopen(filepath, "rb");
        if (!f) continue;

        int size = fread(buffer, 1, sizeof(buffer), f);
        fclose(f);

        if (size < 12) {
            errors++;
            continue;
        }

        char *bufptr = buffer;

        // check machine format
        int ss = MIG_GET_BYTE(bufptr);
        int is = MIG_GET_BYTE(bufptr);
        int ls = MIG_GET_BYTE(bufptr);
        if (ss != 2 || is != 4 || ls != 8) {
            printf("  %s: wrong machine format (%d,%d,%d)\n", de->d_name, ss, is, ls);
            errors++;
            continue;
        }

        int room_vnum = mig_getInt(&bufptr);
        int csize = mig_getInt(&bufptr);

        if (size != csize) {
            printf("  %s: size mismatch (%d vs %d)\n", de->d_name, size, csize);
            errors++;
            continue;
        }

        // parse player name and save_id from filename
        char player_name[64];
        int save_id = 0;
        strncpy(player_name, de->d_name, sizeof(player_name) - 1);
        player_name[sizeof(player_name) - 1] = '\0';

        // find where digits start (timestamp)
        char *p = player_name;
        while (*p && !isdigit(*p)) p++;
        if (*p) {
            save_id = atoi(p);
            *p = '\0';
        }

        if (!player_name[0]) {
            errors++;
            continue;
        }

        // parse objects
        struct mig_obj *items = parse_binary_objects(&bufptr);
        if (!items) {
            errors++;
            continue;
        }

        // insert corpse record (skip if exists)
        char *esc_name = sql_escape_string(player_name);
        char query[512];
        snprintf(query, sizeof(query),
            "INSERT IGNORE INTO corpses (player_name, save_id, room_vnum) VALUES ('%s', %d, %d)",
            esc_name, save_id, room_vnum);
        free(esc_name);

        if (!qry("%s", query)) {
            free_mig_obj(items);
            errors++;
            continue;
        }

        // check if insert happened
        if (mysql_affected_rows(DB) == 0) {
            free_mig_obj(items);
            continue;
        }

        MYSQL_RES *result = db_query("SELECT LAST_INSERT_ID()");
        if (!result) {
            free_mig_obj(items);
            errors++;
            continue;
        }
        MYSQL_ROW row = mysql_fetch_row(result);
        int corpse_id = row ? atoi(row[0]) : 0;
        mysql_free_result(result);

        // save items
        int item_count = 0;
        for (struct mig_obj *obj = items; obj; obj = obj->next) {
            if (save_corpse_item(corpse_id, obj, 0))
                item_count++;
        }
        items_total += item_count;

        count++;

        free_mig_obj(items);
        processed++;
        progress_update(&pb, processed);
    }

    closedir(dir);
    progress_finish(&pb);
    printf("corpses: %d corpses, %d items, %d errors\n", count, items_total, errors);
    return count;
}

// save a saved item
static int save_saved_item(const char *item_key, int room_vnum, struct mig_obj *obj, int container_id) {
    if (!obj) return 0;

    char *esc_key = sql_escape_string(item_key);
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
        "INSERT INTO saved_items ("
        "item_key, room_vnum, vnum, container_id, "
        "weight, cost, timer, extra_flags, "
        "value0, value1, value2, value3, value4, value5, value6, value7, "
        "name, short_descr, description, action_descr"
        ") VALUES ("
        "'%s', %d, %d, %s, "
        "%d, %d, %ld, %lu, "
        "%d, %d, %d, %d, %d, %d, %d, %d, "
        "%s, %s, %s, %s"
        ")",
        esc_key, room_vnum, obj->vnum, container_str,
        obj->weight, obj->cost, obj->timer, obj->extra_flags,
        obj->value[0], obj->value[1], obj->value[2], obj->value[3],
        obj->value[4], obj->value[5], obj->value[6], obj->value[7],
        name_str, short_str, desc_str, action_str
    );

    if (esc_key) free(esc_key);
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

    // save contained objects
    for (struct mig_obj *c = obj->contains; c; c = c->next) {
        save_saved_item(item_key, room_vnum, c, item_id);
    }

    return item_id;
}

int migrate_saved_items_from_files(void) {
    int count = 0;
    int errors = 0;
    char buffer[240000];

    printf("migrating saved items from flatfiles...\n");

    DIR *dir = opendir("Players/SavedItems");
    if (!dir) {
        printf("  no Players/SavedItems directory\n");
        return 0;
    }

    struct dirent *de;
    while ((de = readdir(dir))) {
        // files start with "item."
        if (strstr(de->d_name, "item.") != de->d_name)
            continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "Players/SavedItems/%s", de->d_name);

        FILE *f = fopen(filepath, "rb");
        if (!f) continue;

        int size = fread(buffer, 1, sizeof(buffer), f);
        fclose(f);

        if (size < 8) {
            errors++;
            continue;
        }

        char *bufptr = buffer;

        int room_vnum = mig_getInt(&bufptr);
        int csize = mig_getInt(&bufptr);

        if (size != csize) {
            printf("  %s: size mismatch (%d vs %d)\n", de->d_name, size, csize);
            errors++;
            continue;
        }

        struct mig_obj *items = parse_binary_objects(&bufptr);
        if (!items) {
            errors++;
            continue;
        }

        // save each root item with the filename as key
        for (struct mig_obj *obj = items; obj; obj = obj->next) {
            if (save_saved_item(de->d_name, room_vnum, obj, 0))
                count++;
        }

        printf("  migrated saved item: %s (room %d)\n", de->d_name, room_vnum);

        free_mig_obj(items);
    }

    closedir(dir);
    printf("saved items migration complete: %d items, %d errors\n", count, errors);
    return count;
}

// count recipe files
static int count_recipe_files(void) {
    int total = 0;
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Players/Tradeskills/%c", letter);
        DIR *dir = opendir(dirname);
        if (!dir) continue;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            char *ext = strrchr(entry->d_name, '.');
            if (!ext || strcmp(ext, ".crafting") != 0) continue;
            total++;
        }
        closedir(dir);
    }
    return total;
}

int migrate_recipes_from_files(void) {
    int count = 0;
    int errors = 0;
    int skipped = 0;
    int processed = 0;

    int total = count_recipe_files();
    struct progress_bar pb;
    progress_init(&pb, total, "recipes");

    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Players/Tradeskills/%c", letter);

        DIR *dir = opendir(dirname);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            char *ext = strrchr(entry->d_name, '.');
            if (!ext || strcmp(ext, ".crafting") != 0) continue;

            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);

            // get player name (filename without .crafting)
            char playername[256];
            strncpy(playername, entry->d_name, sizeof(playername) - 1);
            playername[sizeof(playername) - 1] = '\0';
            char *dot = strrchr(playername, '.');
            if (dot) *dot = '\0';

            // look up pid from player_data
            int pid = sql_get_pid_by_name(playername);
            if (pid <= 0) {
                skipped++;
                processed++;
                progress_update(&pb, processed);
                continue;
            }

            FILE *f = fopen(filepath, "r");
            if (!f) {
                errors++;
                processed++;
                progress_update(&pb, processed);
                continue;
            }

            int recipe_count = 0;
            int vnum;
            while (fscanf(f, "%d\n", &vnum) == 1) {
                char query[256];
                snprintf(query, sizeof(query),
                    "INSERT IGNORE INTO player_recipes (pid, recipe_vnum) VALUES (%d, %d)",
                    pid, vnum);
                qry("%s", query);
                recipe_count++;
            }

            fclose(f);

            if (recipe_count > 0) {
                count++;
            }

            processed++;
            progress_update(&pb, processed);
        }

        closedir(dir);
    }

    progress_finish(&pb);
    printf("recipes: %d players, %d skipped (no pid), %d errors\n", count, skipped, errors);
    return count;
}

// count spellbook files
static int count_spellbook_files(void) {
    int total = 0;
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirpath[256];
        snprintf(dirpath, sizeof(dirpath), "Players/%c", letter);
        DIR *dir = opendir(dirpath);
        if (!dir) continue;
        struct dirent *entry;
        while ((entry = readdir(dir))) {
            char *ext = strrchr(entry->d_name, '.');
            if (!ext || strcmp(ext, ".spellbook") != 0) continue;
            total++;
        }
        closedir(dir);
    }
    return total;
}

int migrate_spellbooks_from_files(void) {
    int count = 0;
    int errors = 0;
    int total_mobs = 0;
    int processed = 0;

    int total = count_spellbook_files();
    struct progress_bar pb;
    progress_init(&pb, total, "spellbooks");

    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirpath[256];
        snprintf(dirpath, sizeof(dirpath), "Players/%c", letter);

        DIR *dir = opendir(dirpath);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir))) {
            char *ext = strrchr(entry->d_name, '.');
            if (!ext || strcmp(ext, ".spellbook") != 0)
                continue;

            // get player name (filename without extension)
            char player_name[64];
            strncpy(player_name, entry->d_name, sizeof(player_name) - 1);
            player_name[sizeof(player_name) - 1] = 0;
            char *dot = strrchr(player_name, '.');
            if (dot) *dot = 0;

            // look up pid from database
            char query[256];
            snprintf(query, sizeof(query),
                "SELECT pid FROM player_data WHERE LOWER(name) = '%s'", player_name);
            MYSQL_RES *result = db_query(query);
            if (!result) {
                errors++;
                processed++;
                progress_update(&pb, processed);
                continue;
            }
            MYSQL_ROW row = mysql_fetch_row(result);
            if (!row || !row[0]) {
                mysql_free_result(result);
                errors++;
                processed++;
                progress_update(&pb, processed);
                continue;
            }
            int pid = atoi(row[0]);
            mysql_free_result(result);

            // read spellbook file
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
            FILE *f = fopen(filepath, "r");
            if (!f) {
                errors++;
                processed++;
                progress_update(&pb, processed);
                continue;
            }

            // delete existing spellbook entries for this player
            snprintf(query, sizeof(query),
                "DELETE FROM player_spellbooks WHERE pid = %d", pid);
            qry("%s", query);

            // read mob vnums and insert
            int mob_vnum;
            int mob_count = 0;
            while (fscanf(f, "%d", &mob_vnum) == 1) {
                snprintf(query, sizeof(query),
                    "INSERT IGNORE INTO player_spellbooks (pid, mob_vnum) VALUES (%d, %d)",
                    pid, mob_vnum);
                qry("%s", query);
                mob_count++;
                total_mobs++;
            }
            fclose(f);

            count++;
            processed++;
            progress_update(&pb, processed);
        }
        closedir(dir);
    }

    progress_finish(&pb);
    printf("spellbooks: %d migrated, %d mobs total, %d errors\n", count, total_mobs, errors);
    return count;
}
