// migrate_shopkeepers.c
// shopkeeper pfile migration

#include "migrate_common.h"

static int save_shopkeeper_item(int shopkeeper_id, struct mig_obj *obj, int equip_slot, int container_id) {
    if (!obj) return 0;
    return save_item_to_db(obj, "shopkeeper_items", "shopkeeper_id", shopkeeper_id, container_id, equip_slot);
}

static int count_shopkeeper_files(void) {
    int total = 0;
    DIR *dir = opendir("Players/ShopKeepers");
    if (!dir) return 0;
    struct dirent *de;
    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.') continue;
        total++;
    }
    closedir(dir);
    return total;
}

// parse affects section
static struct mig_affect *parse_shopkeeper_affects(char **buf) {
    int aff_vers = MIG_GET_BYTE(*buf);
    if (aff_vers > SAV_AFFVERS) {
        printf("  bad affect version %d\n", aff_vers);
        return NULL;
    }

    int count = mig_getShort(buf);
    struct mig_affect *head = NULL;
    struct mig_affect *tail = NULL;

    for (int i = 0; i < count; i++) {
        struct mig_affect *af = (struct mig_affect *)malloc(sizeof(struct mig_affect));
        memset(af, 0, sizeof(struct mig_affect));

        int custom_msg = 0;
        if (aff_vers > 5) {
            custom_msg = MIG_GET_BYTE(*buf);
            if (custom_msg & 1) af->wear_off_char = mig_getString(buf);
            if (custom_msg & 2) af->wear_off_room = mig_getString(buf);
            af->type = mig_getShort(buf);
        } else if (aff_vers > 4) {
            af->type = mig_getInt(buf);
        } else {
            af->type = mig_getShort(buf);
        }

        af->duration = mig_getInt(buf);
        af->flags = mig_getShort(buf);
        af->modifier = mig_getInt(buf);
        af->location = MIG_GET_BYTE(*buf);
        af->bitvector1 = mig_getLong(buf);
        af->bitvector2 = mig_getLong(buf);
        af->bitvector3 = mig_getLong(buf);
        af->bitvector4 = mig_getLong(buf);
        af->bitvector5 = mig_getLong(buf);
        mig_getLong(buf);

        if (aff_vers > 7) {
            af->level = mig_getShort(buf);
        }

        af->next = NULL;
        if (!head) head = af;
        if (tail) tail->next = af;
        tail = af;
    }

    return head;
}

// skip pet status section - we dont need it, mob loads from zone
static void skip_pet_status(char **buf) {
    char *s;
    s = mig_getString(buf); if (s) free(s);
    s = mig_getString(buf); if (s) free(s);
    s = mig_getString(buf); if (s) free(s);
    s = mig_getString(buf); if (s) free(s);

    MIG_GET_BYTE(*buf);
    MIG_GET_BYTE(*buf);
    MIG_GET_BYTE(*buf);
    MIG_GET_BYTE(*buf);

    mig_getShort(buf);
    mig_getShort(buf);

    MIG_GET_BYTE(*buf);

    mig_getInt(buf);
    mig_getInt(buf);

    for (int i = 0; i < 10; i++) MIG_GET_BYTE(*buf);

    mig_getShort(buf);
    mig_getShort(buf);
    mig_getShort(buf);
    mig_getShort(buf);
    mig_getShort(buf);
    mig_getShort(buf);
}

int migrate_shopkeepers_from_files(void) {
    int count = 0;
    int errors = 0;
    int items_total = 0;
    char buffer[240000];

    int total = count_shopkeeper_files();
    struct progress_bar pb;
    progress_init(&pb, total, "shopkeepers");

    DIR *dir = opendir("Players/ShopKeepers");
    if (!dir) {
        printf("  Players/ShopKeepers not found\n");
        progress_finish(&pb);
        return 0;
    }

    struct dirent *de;
    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.') continue;

        int shop_nr = atoi(de->d_name);
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "Players/ShopKeepers/%s", de->d_name);

        FILE *f = fopen(filepath, "rb");
        if (!f) continue;

        int size = fread(buffer, 1, sizeof(buffer), f);
        fclose(f);

        if (size < 20) {
            errors++;
            progress_update(&pb, count + errors);
            continue;
        }

        char *bufptr = buffer;

        int ss = MIG_GET_BYTE(bufptr);
        int is = MIG_GET_BYTE(bufptr);
        int ls = MIG_GET_BYTE(bufptr);
        if (ss != 2 || is != 4 || ls != 8) {
            printf("  %s: wrong machine format\n", de->d_name);
            errors++;
            progress_update(&pb, count + errors);
            continue;
        }

        int affect_off = mig_getInt(&bufptr);
        int item_off = mig_getInt(&bufptr);
        int csize = mig_getInt(&bufptr);
        int mob_vnum = mig_getInt(&bufptr);
        long save_time = mig_getLong(&bufptr);
        int room_vnum = mig_getInt(&bufptr);

        // detect and fix corrupted mob_vnum (some files have extra byte causing 8-bit shift)
        // if mob_vnum is way off from room_vnum but shifting right by 8 brings them close, apply fix
        int diff = mob_vnum > room_vnum ? mob_vnum - room_vnum : room_vnum - mob_vnum;
        if (diff > 50000) {
            int corrected = mob_vnum >> 8;
            int corr_diff = corrected > room_vnum ? corrected - room_vnum : room_vnum - corrected;
            if (corr_diff < 1000) {
                mob_vnum = corrected;
            }
        }

        if (size != csize) {
            printf("  %s: size mismatch %d vs %d\n", de->d_name, size, csize);
            errors++;
            progress_update(&pb, count + errors);
            continue;
        }

        skip_pet_status(&bufptr);

        char *aff_ptr = buffer + affect_off;
        struct mig_affect *affects = parse_shopkeeper_affects(&aff_ptr);

        char *item_ptr = buffer + item_off;
        struct mig_obj *items = parse_binary_objects(&item_ptr);

        qry("DELETE FROM shopkeepers WHERE shop_id=%d", shop_nr);

        char query[512];
        snprintf(query, sizeof(query),
            "INSERT INTO shopkeepers (shop_id, mob_vnum, room_vnum, save_time) VALUES (%d, %d, %d, FROM_UNIXTIME(NULLIF(%ld,0)))",
            shop_nr, mob_vnum, room_vnum, save_time);

        if (!qry("%s", query)) {
            printf("  %s: insert failed\n", de->d_name);
            errors++;
            free_mig_affect(affects);
            free_mig_obj(items);
            progress_update(&pb, count + errors);
            continue;
        }

        MYSQL_RES *res = db_query("SELECT LAST_INSERT_ID()");
        int shopkeeper_id = 0;
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row) shopkeeper_id = atoi(row[0]);
            mysql_free_result(res);
        }

        for (struct mig_affect *af = affects; af; af = af->next) {
            qry("INSERT INTO shopkeeper_affects (shopkeeper_id, type, duration, modifier, location, "
                "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5) VALUES "
                "(%d, %d, %d, %d, %d, %ld, %ld, %ld, %ld, %ld)",
                shopkeeper_id, af->type, af->duration, af->modifier, af->location,
                af->bitvector1, af->bitvector2, af->bitvector3, af->bitvector4, af->bitvector5);
        }

        for (struct mig_obj *obj = items; obj; obj = obj->next) {
            int item_id = save_shopkeeper_item(shopkeeper_id, obj, 0, 0);
            items_total++;

            for (struct mig_obj *c = obj->contains; c; c = c->next) {
                save_shopkeeper_item(shopkeeper_id, c, 0, item_id);
                items_total++;
            }
        }

        free_mig_affect(affects);
        free_mig_obj(items);
        count++;
        progress_update(&pb, count + errors);
    }

    closedir(dir);
    progress_finish(&pb);

    printf("shopkeepers: %d migrated, %d errors, %d items\n", count, errors, items_total);
    return count;
}
