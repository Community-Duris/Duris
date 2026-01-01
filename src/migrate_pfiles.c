// migrate_pfiles.c
// standalone tool to migrate flatfiles to database
// usage: ./migrate_pfiles
//
// this program reads the old flatfile formats and saves to mysql using
// the same sql functions as the mud

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>

#include "structs.h"
#include "utils.h"
#include "sql.h"
#include "sql_player.h"
#include "account.h"
#include "assocs.h"
#include "ships/ships.h"

// external vars we need
P_Guild guild_list = NULL;
char buf[MAX_STRING_LENGTH];

// forward declarations
char *str_dup(const char *source);
void __free(void *p, char *file, int line);
char *sql_escape_string(const char *str);
MYSQL_RES *db_query(const char *format, ...);
bool qry(const char *format, ...);

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

// ============================================================================
// binary object migration
// ============================================================================

// flags from files.h
#define SAV_ITEMVERS  35
#define O_F_WORN      1
#define O_F_CONTAINS  2
#define O_F_UNIQUE    4
#define O_F_COUNT     8
#define O_F_EOL       16
#define O_F_AFFECTS   32
#define O_F_SPELLBOOK 64

#define O_U_KEYS      (1UL << 0)
#define O_U_DESC1     (1UL << 1)
#define O_U_DESC2     (1UL << 2)
#define O_U_DESC3     (1UL << 3)
#define O_U_VAL0      (1UL << 4)
#define O_U_VAL1      (1UL << 5)
#define O_U_VAL2      (1UL << 6)
#define O_U_VAL3      (1UL << 7)
#define O_U_TYPE      (1UL << 8)
#define O_U_WEAR      (1UL << 9)
#define O_U_EXTRA     (1UL << 10)
#define O_U_WEIGHT    (1UL << 11)
#define O_U_COST      (1UL << 12)
#define O_U_BV1       (1UL << 13)
#define O_U_BV2       (1UL << 14)
#define O_U_AFFS      (1UL << 15)
#define O_U_TRAP      (1UL << 16)
#define O_U_COND      (1UL << 17)
#define O_U_ANTI      (1UL << 18)
#define O_U_EXTRA2    (1UL << 19)
#define O_U_TIMER     (1UL << 20)
#define O_U_ANTI2     (1UL << 21)
#define O_U_EDESC     (1UL << 22)
#define O_U_MATERIAL  (1UL << 23)
#define O_U_SPACE     (1UL << 24)
#define O_U_VAL4      (1UL << 25)
#define O_U_VAL5      (1UL << 26)
#define O_U_VAL6      (1UL << 27)
#define O_U_VAL7      (1UL << 28)
#define O_U_BV3       (1UL << 29)
#define O_U_BV4       (1UL << 30)
#define O_U_BV5       (1UL << 31)

// external binary read helpers from migrate_stubs.c
extern ush_int mig_getShort(char **buf);
extern uint mig_getInt(char **buf);
extern long mig_getLong(char **buf);
extern char *mig_getString(char **buf);

#define MIG_GET_BYTE(buf) (*(char *)((buf)++))

// minimal object struct for migration
struct mig_obj {
    int vnum;
    int weight;
    int cost;
    long timer;
    unsigned long extra_flags;
    int value[8];
    char *name;
    char *short_descr;
    char *description;
    char *action_descr;
    struct mig_obj *contains;
    struct mig_obj *next;
};

static void free_mig_obj(struct mig_obj *obj) {
    if (!obj) return;
    if (obj->name) free(obj->name);
    if (obj->short_descr) free(obj->short_descr);
    if (obj->description) free(obj->description);
    if (obj->action_descr) free(obj->action_descr);
    free_mig_obj(obj->contains);
    free_mig_obj(obj->next);
    free(obj);
}

// parse binary object data, returns linked list of mig_obj
// container_stack handles nested containers
static struct mig_obj *parse_binary_objects(char **buf) {
    int obj_vers = MIG_GET_BYTE(*buf);
    if (obj_vers > SAV_ITEMVERS) {
        printf("  error: item version %d > %d\n", obj_vers, SAV_ITEMVERS);
        return NULL;
    }

    int count = mig_getInt(buf);
    struct mig_obj *root = NULL;
    struct mig_obj *last = NULL;
    struct mig_obj *container_stack[32];
    int stack_depth = 0;

    for (;;) {
        unsigned char o_f_flag = MIG_GET_BYTE(*buf);

        if (o_f_flag & O_F_EOL) {
            if (stack_depth > 0) {
                stack_depth--;
                continue;
            }
            break;
        }

        struct mig_obj *obj = (struct mig_obj *)malloc(sizeof(struct mig_obj));
        memset(obj, 0, sizeof(struct mig_obj));

        obj->vnum = mig_getInt(buf);
        mig_getShort(buf); // craftsmanship - skip
        mig_getShort(buf); // condition - skip

        if (o_f_flag & O_F_WORN)
            MIG_GET_BYTE(*buf); // wear location - skip

        if (o_f_flag & O_F_COUNT)
            mig_getShort(buf); // quantity - skip

        if (o_f_flag & O_F_AFFECTS) {
            int aff_count = MIG_GET_BYTE(*buf);
            while (aff_count--) {
                mig_getInt(buf);   // time
                mig_getShort(buf); // type
                mig_getShort(buf); // data
                mig_getInt(buf);   // extra2
            }
        }

        if (o_f_flag & O_F_UNIQUE) {
            unsigned long o_u_flag = mig_getInt(buf);

            if (o_u_flag & O_U_KEYS)
                obj->name = mig_getString(buf);
            if (o_u_flag & O_U_DESC1)
                obj->description = mig_getString(buf);
            if (o_u_flag & O_U_DESC2)
                obj->short_descr = mig_getString(buf);
            if (o_u_flag & O_U_DESC3)
                obj->action_descr = mig_getString(buf);
            if (o_u_flag & O_U_EDESC) {
                int nDescs = mig_getShort(buf);
                while (nDescs--) {
                    char *kw = mig_getString(buf);
                    char *desc = mig_getString(buf);
                    if (kw) free(kw);
                    if (desc) free(desc);
                }
            }
            if (o_u_flag & O_U_VAL0)
                obj->value[0] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL1)
                obj->value[1] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL2)
                obj->value[2] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL3)
                obj->value[3] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL4)
                obj->value[4] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL5)
                obj->value[5] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL6)
                obj->value[6] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL7)
                obj->value[7] = mig_getInt(buf);
            if (o_u_flag & O_U_TIMER) {
                obj->timer = mig_getInt(buf);
                mig_getInt(buf); mig_getInt(buf); mig_getInt(buf); // timer[1-3]
            }
            if (o_u_flag & O_U_TRAP) {
                mig_getShort(buf); mig_getShort(buf);
                mig_getShort(buf); mig_getShort(buf);
            }
            if (o_u_flag & O_U_TYPE)
                MIG_GET_BYTE(*buf);
            if (o_u_flag & O_U_WEAR)
                mig_getInt(buf);
            if (o_u_flag & O_U_EXTRA)
                obj->extra_flags = mig_getInt(buf);
            if (o_u_flag & O_U_ANTI)
                mig_getInt(buf);
            if (o_u_flag & O_U_ANTI2)
                mig_getInt(buf);
            if (o_u_flag & O_U_EXTRA2)
                mig_getInt(buf);
            if (o_u_flag & O_U_WEIGHT)
                obj->weight = mig_getInt(buf);
            if (o_u_flag & O_U_MATERIAL)
                MIG_GET_BYTE(*buf);
            if (o_u_flag & O_U_COST)
                obj->cost = mig_getInt(buf);
            if (o_u_flag & O_U_BV1)
                mig_getLong(buf);
            if (o_u_flag & O_U_BV2)
                mig_getLong(buf);
            if (o_u_flag & O_U_BV3)
                mig_getLong(buf);
            if (o_u_flag & O_U_BV4)
                mig_getLong(buf);
            if (o_u_flag & O_U_BV5)
                mig_getLong(buf);
            if (o_u_flag & O_U_AFFS) {
                for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                    MIG_GET_BYTE(*buf); // location
                    MIG_GET_BYTE(*buf); // modifier
                }
            }
            if (o_f_flag & O_F_SPELLBOOK) {
                int tmp = mig_getInt(buf);
                for (int i = 0; i < tmp; i++)
                    MIG_GET_BYTE(*buf);
            }
        }

        // link to list or container
        if (stack_depth > 0) {
            struct mig_obj *parent = container_stack[stack_depth - 1];
            if (!parent->contains) {
                parent->contains = obj;
            } else {
                struct mig_obj *c = parent->contains;
                while (c->next) c = c->next;
                c->next = obj;
            }
        } else {
            if (!root)
                root = obj;
            if (last)
                last->next = obj;
            last = obj;
        }

        if (o_f_flag & O_F_CONTAINS) {
            container_stack[stack_depth++] = obj;
        }
    }

    return root;
}

// ============================================================================
// account migration - copied from old read_account() in git history
// ============================================================================

// forward declaration for sql save
bool sql_save_account(struct acct_entry *acc);

static void read_unique_ip_file(struct acct_entry *acct, FILE *f) {
    int count = 0;
    struct acct_ip *c = NULL;
    struct acct_ip *d = NULL;
    char hostname[256], ip_address[256];

    fscanf(f, "%d\n", &count);
    acct->num_ips = count;
    if (count == 0)
        return;

    for (int i = 0; i < count; i++) {
        c = (struct acct_ip *)malloc(sizeof(struct acct_ip));
        memset(c, 0, sizeof(struct acct_ip));

        fgets(hostname, sizeof(hostname), f);
        hostname[strcspn(hostname, "\r\n")] = 0;
        c->hostname = str_dup(hostname);

        fgets(ip_address, sizeof(ip_address), f);
        ip_address[strcspn(ip_address, "\r\n")] = 0;
        c->ip_address = str_dup(ip_address);

        fscanf(f, "%lu\n", &c->count);
        c->next = NULL;

        if (i == 0)
            acct->acct_unique_ips = c;
        if (d)
            d->next = c;
        d = c;
    }
}

static void read_character_list_file(struct acct_entry *acct, FILE *f) {
    int count = 0;
    struct acct_chars *c = NULL;
    struct acct_chars *d = NULL;
    char charname[256];

    fscanf(f, "%d\n", &count);
    acct->num_chars = count;
    if (count == 0)
        return;

    for (int i = 0; i < count; i++) {
        c = (struct acct_chars *)malloc(sizeof(struct acct_chars));
        memset(c, 0, sizeof(struct acct_chars));

        fscanf(f, "%s\n", charname);
        c->charname = str_dup(charname);
        fscanf(f, "%lu %ld %hhd %hhd\n", &c->count, &c->last, &c->blocked, &c->racewar);
        c->next = NULL;

        if (i == 0)
            acct->acct_character_list = c;
        if (d)
            d->next = c;
        d = c;
    }
}

static void free_account(struct acct_entry *acct) {
    if (!acct) return;

    if (acct->acct_name) free(acct->acct_name);
    if (acct->acct_email) free(acct->acct_email);
    if (acct->acct_password) free(acct->acct_password);
    if (acct->acct_confirmation) free(acct->acct_confirmation);

    struct acct_ip *ip = acct->acct_unique_ips;
    while (ip) {
        struct acct_ip *next = ip->next;
        if (ip->hostname) free(ip->hostname);
        if (ip->ip_address) free(ip->ip_address);
        free(ip);
        ip = next;
    }

    struct acct_chars *ch = acct->acct_character_list;
    while (ch) {
        struct acct_chars *next = ch->next;
        if (ch->charname) free(ch->charname);
        free(ch);
        ch = next;
    }

    free(acct);
}

static int migrate_accounts_from_files(void) {
    int count = 0;
    int errors = 0;

    printf("migrating accounts from flatfiles...\n");

    // iterate through Accounts/a-z directories
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Accounts/%c", letter);

        DIR *dir = opendir(dirname);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);

            // check if it's a file
            struct stat st;
            if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode)) continue;

            FILE *f = fopen(filepath, "r");
            if (!f) {
                printf("  error: could not open %s\n", filepath);
                errors++;
                continue;
            }

            struct acct_entry *acct = (struct acct_entry *)malloc(sizeof(struct acct_entry));
            memset(acct, 0, sizeof(struct acct_entry));

            char line[4096];
            int serial = 0;

            // read serial number
            fscanf(f, "%d\n", &serial);

            // read account name
            fgets(line, sizeof(line), f);
            line[strcspn(line, "\r\n")] = 0;
            acct->acct_name = str_dup(line);

            // read email
            fgets(line, sizeof(line), f);
            line[strcspn(line, "\r\n")] = 0;
            acct->acct_email = str_dup(line);

            // read password
            fgets(line, sizeof(line), f);
            line[strcspn(line, "\r\n")] = 0;
            acct->acct_password = str_dup(line);

            // read confirmation code
            fgets(line, sizeof(line), f);
            line[strcspn(line, "\r\n")] = 0;
            acct->acct_confirmation = str_dup(line);

            // read ips and characters
            read_unique_ip_file(acct, f);
            read_character_list_file(acct, f);

            // read remaining fields
            fscanf(f, "%hhd\n", &acct->acct_blocked);
            fscanf(f, "%hhd\n", &acct->acct_confirmed);
            fscanf(f, "%hhd\n", &acct->acct_confirmation_sent);
            fscanf(f, "%li\n", &acct->acct_last);
            fscanf(f, "%li\n", &acct->acct_good);
            fscanf(f, "%li\n", &acct->acct_evil);
            fscanf(f, "%li\n", &acct->acct_flags1);
            fscanf(f, "%li\n", &acct->acct_flags2);
            fscanf(f, "%li\n", &acct->acct_flags3);
            fscanf(f, "%li\n", &acct->acct_flags4);

            fclose(f);

            // save to database
            if (sql_save_account(acct)) {
                printf("  migrated account: %s (%d chars, %d ips)\n",
                    acct->acct_name, acct->num_chars, acct->num_ips);
                count++;
            } else {
                printf("  error: failed to save account %s to database\n", acct->acct_name);
                errors++;
            }

            free_account(acct);
        }

        closedir(dir);
    }

    printf("accounts migration complete: %d migrated, %d errors\n", count, errors);
    return count;
}

// ============================================================================
// recipe/tradeskill migration
// ============================================================================

// forward declaration
static int sql_get_pid_by_name(const char *name);

static int migrate_recipes_from_files(void) {
    int count = 0;
    int errors = 0;
    int skipped = 0;

    printf("migrating recipes from flatfiles...\n");

    // iterate through Players/Tradeskills/a-z directories
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Players/Tradeskills/%c", letter);

        DIR *dir = opendir(dirname);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            // check for .crafting extension
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
                continue;
            }

            FILE *f = fopen(filepath, "r");
            if (!f) {
                errors++;
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
                printf("  migrated %d recipes for %s (pid %d)\n", recipe_count, playername, pid);
                count++;
            }
        }

        closedir(dir);
    }

    printf("recipes migration complete: %d players, %d skipped (no pid), %d errors\n", count, skipped, errors);
    return count;
}

static int sql_get_pid_by_name(const char *name) {
    if (!name) return 0;

    char *esc_name = sql_escape_string(name);
    if (!esc_name) return 0;

    char query[256];
    snprintf(query, sizeof(query),
        "SELECT pid FROM player_data WHERE LOWER(name) = LOWER('%s') LIMIT 1", esc_name);
    free(esc_name);

    MYSQL_RES *result = db_query("%s", query);
    if (!result) return 0;

    MYSQL_ROW row = mysql_fetch_row(result);
    int pid = row ? atoi(row[0]) : 0;
    mysql_free_result(result);

    return pid;
}

// ============================================================================
// ship migration - copied from old read_ships() in git history
// ============================================================================

static int migrate_ships_from_files(void) {
    P_ship ship;
    char *ret = NULL;
    int k, ver;
    FILE *f = NULL, *f2 = NULL;
    int count = 0;
    int errors = 0;

    printf("migrating ships from flatfiles...\n");

    f = fopen("Players/Ships/ship_index", "r");
    if (!f) {
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
            printf("  error: could not open %s\n", buf);
            errors++;
            continue;
        }

        if ((k = fscanf(f2, "version:%d\n", &ver)) != 1)
            ver = 0;

        if (ver != 3) {
            printf("  error: unknown version %d in %s\n", ver, buf);
            fclose(f2);
            errors++;
            continue;
        }

        fscanf(f2, "%d\n", &k);
        ship = new_ship(k, false);

        if (!ship) {
            printf("  error: could not create ship class %d\n", k);
            fclose(f2);
            errors++;
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
            printf("  migrated ship: %s (owner: %s)\n", ship->name, ship->ownername);
            count++;
        } else {
            printf("  error: failed to save ship %s to database\n", ship->ownername);
            errors++;
        }

        // free the ship - we don't need it in memory
        FREE(ship->ownername);
        FREE(ship->name);
        FREE(ship);
    }

    FREE(ret);
    fclose(f);

    printf("ships migration complete: %d migrated, %d errors\n", count, errors);
    return count;
}

// ============================================================================
// guild migration - copied from old Guild::load_guild() in git history
// ============================================================================

int migrate_guilds_from_files(void) {
    FILE *file;
    char filename[300], buf[300], mem_name[MAX_NAME_LENGTH + 1];
    int mem_bits, mem_debt;
    int count = 0;
    int errors = 0;
    int miss_count = 0;

    printf("migrating guilds from flatfiles...\n");

    for (int guild_num = 1; miss_count < 20; guild_num++) {
        snprintf(filename, sizeof(filename), "Players/Assocs/asc.%d", guild_num);
        file = fopen(filename, "r");

        if (!file) {
            miss_count++;
            continue;
        }
        miss_count = 0;

        Guild *new_guild = new Guild();

        // get the guild name
        fgets(new_guild->name, ASC_MAX_STR, file);
        // cut the carriage return off
        char *nl = strchr(new_guild->name, '\n');
        if (nl) *nl = '\0';

        // then the guild number and frag info
        fscanf(file, "%u %lu %lu %s\n",
            &(new_guild->racewar),
            &(new_guild->frags.frags),
            &(new_guild->frags.top_frags),
            new_guild->frags.topfragger);

        new_guild->id_number = guild_num;

        // then get the default guild titles
        for (int i = 0; i < ASC_NUM_RANKS; i++) {
            fgets(buf, ASC_MAX_STR_RANK + 1, file);
            // cut the carriage return off
            buf[strlen(buf) - 1] = '\0';
            snprintf(new_guild->titles[i], ASC_MAX_STR_RANK, "%s", buf);
        }

        // then get the guild bits, prestige and construction
        fgets(buf, sizeof(buf), file);
        sscanf(buf, "%u %lu %lu\n",
            &new_guild->bits,
            &new_guild->prestige,
            &new_guild->construction);

        // then get the money for the guild
        fscanf(file, "%u %u %u %u\n",
            &(new_guild->platinum),
            &(new_guild->gold),
            &(new_guild->silver),
            &(new_guild->copper));

        // then get members
        new_guild->members = NULL;
        new_guild->member_count = 0;
        P_member last_member = NULL;

        while (fscanf(file, "%s %u %u\n", mem_name, (unsigned int*)&mem_bits, (unsigned int*)&mem_debt) == 3) {
            P_member new_member = new guild_member();
            snprintf(new_member->name, MAX_NAME_LENGTH + 1, "%s", mem_name);
            new_member->bits = mem_bits;
            new_member->debt = mem_debt;
            new_member->next = NULL;

            if (last_member == NULL) {
                new_guild->members = new_member;
            } else {
                last_member->next = new_member;
            }
            last_member = new_member;
            new_guild->member_count++;
        }

        fclose(file);

        // save to database
        if (sql_save_guild(new_guild)) {
            printf("  migrated guild %d: %s (%d members)\n",
                guild_num, new_guild->name, new_guild->member_count);
            count++;
        } else {
            printf("  error: failed to save guild %d to database\n", guild_num);
            errors++;
        }

        // free the guild
        P_member mem = new_guild->members;
        while (mem) {
            P_member next = mem->next;
            delete mem;
            mem = next;
        }
        delete new_guild;
    }

    printf("guilds migration complete: %d migrated, %d errors\n", count, errors);
    return count;
}

// ============================================================================
// corpse migration
// ============================================================================

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

static int migrate_corpses_from_files(void) {
    int count = 0;
    int errors = 0;
    int items_total = 0;
    char buffer[240000];

    printf("migrating corpses from flatfiles...\n");

    DIR *dir = opendir("Players/Corpses");
    if (!dir) {
        printf("  no Players/Corpses directory\n");
        return 0;
    }

    struct dirent *de;
    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.') continue;

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

        char *buf = buffer;

        // check machine format
        int ss = MIG_GET_BYTE(buf);
        int is = MIG_GET_BYTE(buf);
        int ls = MIG_GET_BYTE(buf);
        if (ss != 2 || is != 4 || ls != 8) {
            printf("  %s: wrong machine format (%d,%d,%d)\n", de->d_name, ss, is, ls);
            errors++;
            continue;
        }

        int room_vnum = mig_getInt(&buf);
        int csize = mig_getInt(&buf);

        if (size != csize) {
            printf("  %s: size mismatch (%d vs %d)\n", de->d_name, size, csize);
            errors++;
            continue;
        }

        // parse player name and save_id from filename (format: <name><timestamp>)
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
        struct mig_obj *items = parse_binary_objects(&buf);
        if (!items) {
            errors++;
            continue;
        }

        // insert corpse record
        char *esc_name = sql_escape_string(player_name);
        char query[512];
        snprintf(query, sizeof(query),
            "INSERT INTO corpses (player_name, save_id, room_vnum) VALUES ('%s', %d, %d)",
            esc_name, save_id, room_vnum);
        free(esc_name);

        if (!qry("%s", query)) {
            free_mig_obj(items);
            errors++;
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

        printf("  migrated corpse: %s (save_id %d, room %d, %d items)\n",
            player_name, save_id, room_vnum, item_count);
        count++;

        free_mig_obj(items);
    }

    closedir(dir);
    printf("corpses migration complete: %d corpses, %d items, %d errors\n", count, items_total, errors);
    return count;
}

// ============================================================================
// saved items migration
// ============================================================================

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

static int migrate_saved_items_from_files(void) {
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

        char *buf = buffer;

        // no machine format check for saved items
        int room_vnum = mig_getInt(&buf);
        int csize = mig_getInt(&buf);

        if (size != csize) {
            printf("  %s: size mismatch (%d vs %d)\n", de->d_name, size, csize);
            errors++;
            continue;
        }

        struct mig_obj *items = parse_binary_objects(&buf);
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

// ============================================================================
// main
// ============================================================================

int main(int argc, char **argv) {
    printf("=== pfile migration tool ===\n\n");

    // load env file if exists
    FILE *env = fopen(".env", "r");
    if (env) {
        char line[256];
        while (fgets(line, sizeof(line), env)) {
            // skip comments and empty lines
            if (line[0] == '#' || line[0] == '\n') continue;

            // remove newline
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';

            // parse KEY=VALUE
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                setenv(line, eq + 1, 1);
            }
        }
        fclose(env);
        printf("loaded .env file\n");
    }

    printf("connecting to database: %s@%s/%s\n",
        get_db_user(), get_db_host(), get_db_name());

    if (!initialize_mysql()) {
        printf("error: failed to connect to database\n");
        return 1;
    }
    printf("database connected\n\n");

    int accounts = migrate_accounts_from_files();
    printf("\n");
    int recipes = migrate_recipes_from_files();
    printf("\n");
    int ships = migrate_ships_from_files();
    printf("\n");
    int guilds = migrate_guilds_from_files();
    printf("\n");
    int corpses = migrate_corpses_from_files();
    printf("\n");
    int saved_items = migrate_saved_items_from_files();

    printf("\n=== migration complete ===\n");
    printf("accounts: %d\n", accounts);
    printf("recipes: %d\n", recipes);
    printf("ships: %d\n", ships);
    printf("guilds: %d\n", guilds);
    printf("corpses: %d\n", corpses);
    printf("saved items: %d\n", saved_items);

    return 0;
}
