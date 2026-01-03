// migrate_ships.c
// ship migration for pfile migration tool

#include "migrate_common.h"

// count ships in index file
static int count_ships(void) {
    FILE *f = fopen("Players/Ships/ship_index", "r");
    if (!f) return 0;
    int total = 0;
    char *ret = fread_string(f);
    while (*ret != '$') {
        FREE(ret);
        ret = fread_string(f);
        total++;
    }
    FREE(ret);
    fclose(f);
    return total;
}

int migrate_ships_from_files(void) {
    P_ship ship;
    char *ret = NULL;
    int k, ver;
    FILE *f = NULL, *f2 = NULL;
    int count = 0;
    int errors = 0;
    int processed = 0;

    int total = count_ships();
    struct progress_bar pb;
    progress_init(&pb, total, "ships");

    f = fopen("Players/Ships/ship_index", "r");
    if (!f) {
        progress_finish(&pb);
        printf("error: could not open Players/Ships/ship_index\n");
        return -1;
    }

    ret = fread_string(f);
    while (*ret != '$') {
        sprintf(buf, "Players/Ships/%s", ret);
        FREE(ret);
        ret = fread_string(f);

        f2 = fopen(buf, "r");
        if (!f2) {
            errors++;
            processed++;
            progress_update(&pb, processed);
            continue;
        }

        if ((k = fscanf(f2, "version:%d\n", &ver)) != 1)
            ver = 0;

        if (ver != 3) {
            fclose(f2);
            errors++;
            processed++;
            progress_update(&pb, processed);
            continue;
        }

        fscanf(f2, "%d\n", &k);
        ship = new_ship(k, false);

        if (!ship) {
            fclose(f2);
            errors++;
            processed++;
            progress_update(&pb, processed);
            continue;
        }

        // read owner name
        fgets(buf, MAX_STRING_LENGTH, f2);
        for (int i = 0; buf[i] != '\0'; i++)
            if (buf[i] == '\n') { buf[i] = '\0'; break; }
        ship->ownername = str_dup(buf);

        // read ship name
        fgets(buf, MAX_STRING_LENGTH, f2);
        for (int i = 0; buf[i] != '\0'; i++)
            if (buf[i] == '\n') { buf[i] = '\0'; break; }
        ship->name = str_dup(buf);

        // frags, anchor, time
        fscanf(f2, "%d\n", &(ship->frags));
        fscanf(f2, "%d %d\n", &(ship->anchor), &(ship->time));

        // armor per side
        for (int i = 0; i < 4; i++) {
            fscanf(f2, "%d %d\n", &(ship->armor[i]), &(ship->internal[i]));
        }

        // mainsail
        fscanf(f2, "%d\n", &(ship->mainsail));
        // bound mainsail to 0..max
        int maxsail = SHIPTYPE_MAX_SAIL(ship->m_class);
        if (ship->mainsail < 0) ship->mainsail = 0;
        if (ship->mainsail > maxsail) ship->mainsail = maxsail;

        // crew data
        int dummy, ss, gs, rs;
        fscanf(f2, "%d\n", &(ship->crew.index));
        fscanf(f2, "%d %d %d %d %d %d\n", &ss, &gs, &rs, &dummy, &dummy, &dummy);
        ship->crew.sail_skill = (float)ss / 1000;
        ship->crew.guns_skill = (float)gs / 1000;
        ship->crew.rpar_skill = (float)rs / 1000;
        fscanf(f2, "%d %d %d %d %d %d %d %d %d %d\n",
            &(ship->crew.sail_chief), &(ship->crew.guns_chief), &(ship->crew.rpar_chief),
            &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);

        // clear and read slots
        for (int i = 0; i < MAXSLOTS; i++)
            ship->slot[i].clear();

        for (int i = 0; i < MAXSLOTS; i++) {
            if (fscanf(f2, "%d %d\n",
                &(ship->slot[i].type),
                &(ship->slot[i].index)) != 2) {
                break;
            }
            fscanf(f2, "%d %d\n",
                &(ship->slot[i].position),
                &(ship->slot[i].timer));
            fscanf(f2, "%d %d %d %d %d\n",
                &(ship->slot[i].val0),
                &(ship->slot[i].val1),
                &(ship->slot[i].val2),
                &(ship->slot[i].val3),
                &(ship->slot[i].val4));

            if (ship->slot[i].type == SLOT_WEAPON) {
                if (ship->slot[i].timer < 0)
                    ship->slot[i].timer = 0;
                ship->slot[i].val3 = -1;
                ship->slot[i].val4 = -1;
            } else if (ship->slot[i].type == SLOT_CARGO || ship->slot[i].type == SLOT_CONTRABAND) {
                ship->slot[i].val2 = -1;
                ship->slot[i].val3 = -1;
                ship->slot[i].val4 = -1;
            }
        }

        fclose(f2);

        // save to database
        if (sql_save_ship(ship)) {
            count++;
        } else {
            errors++;
        }

        // free the ship - we don't need it in memory
        FREE(ship->ownername);
        FREE(ship->name);
        FREE(ship);

        processed++;
        progress_update(&pb, processed);
    }

    FREE(ret);
    fclose(f);

    progress_finish(&pb);
    printf("ships: %d migrated, %d errors\n", count, errors);
    return count;
}
