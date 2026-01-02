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

// constants from config.h that we need
#define MAX_OBJ_AFFECT 4

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

// version constants from files.h
#define SAV_SAVEVERS  5
#define SAV_STATVERS  47
#define SAV_SKILLVERS 2
#define SAV_AFFVERS   8
#define SAV_WTNSVERS  2
#define SAV_ITEMVERS  35

// array size constants
#define MIG_MAX_SKILLS      2000
#define MIG_MAX_TONGUE      29
#define MIG_MAX_INTRO       150
#define MIG_MAX_FORGE_ITEMS 1000
#define MIG_NUMB_PC_TIMERS  10
#define MIG_MAX_CIRCLE      12
#define MIG_MAX_COND        5
#define MIG_MAX_WEAR        22

// object flags
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
extern MYSQL *DB;

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

// affect struct for player migration
struct mig_affect {
    short type;
    int duration;
    short flags;
    int modifier;
    unsigned char location;
    long bitvector1, bitvector2, bitvector3, bitvector4, bitvector5;
    short level;
    char *wear_off_char;
    char *wear_off_room;
    struct mig_affect *next;
};

// player struct for migration
struct mig_player {
    char name[64];
    int pid;
    char password[64];
    char *short_descr;
    char *long_descr;
    char *description;
    char *title;
    unsigned int m_class, secondary_class;
    int spec;
    int race, racewar, level, sex;
    int weight, height, size;
    int hometown, birthplace, orig_birthplace;
    long birth_time, played_time, last_save;
    int perm_aging;

    // stats
    int str, dex, agi, con, pow, intel, wis, cha, kar, luk;

    // points
    int mana, base_mana, hp, base_hp, vitality, base_vitality;
    int spells_memmed;

    // money
    int copper, silver, gold, platinum;
    int bank_copper, bank_silver, bank_gold, bank_platinum;

    // experience
    int exp, epics, epic_skill_points, skillpoints;
    int spell_bind_used;

    // flags
    unsigned int act, act2;
    int vote, alignment;
    int prestige, assoc_id, guild_status;
    long time_left_guild;
    int nb_left_guild;
    long time_unspecced;
    long frags, oldfrags;
    long numb_deaths;

    // conditions
    int conditions[MIG_MAX_COND];

    // immortal
    char *poof_in, *poof_out, *poof_in_sound, *poof_out_sound;
    int echo_toggle, prompt;
    long wiz_invis;
    unsigned long law_flags;
    int wimpy, aggressive, highest_level, screen_length;

    // quest
    int quest_active, quest_mob_vnum, quest_type, quest_accomplished;
    int quest_started, quest_zone_number, quest_giver, quest_level;
    int quest_receiver, quest_shares_left, quest_kill_how_many, quest_kill_original;
    int quest_map_room, quest_map_bought;

    // arrays
    int languages[MIG_MAX_TONGUE];
    long timers[MIG_NUMB_PC_TIMERS];
    int undead_slots[MIG_MAX_CIRCLE + 1];
    int forged_items[MIG_MAX_FORGE_ITEMS];
    int *granted_cmds;
    int num_granted_cmds;

    // intros
    int intro_pids[MIG_MAX_INTRO];
    long intro_times[MIG_MAX_INTRO];

    // skills
    unsigned char skills_learned[MIG_MAX_SKILLS];
    unsigned char skills_taught[MIG_MAX_SKILLS];

    // affects
    struct mig_affect *affects;

    // items
    struct mig_obj *equipment[MIG_MAX_WEAR];
    struct mig_obj *inventory;
};

static void free_mig_affect(struct mig_affect *af) {
    while (af) {
        struct mig_affect *next = af->next;
        if (af->wear_off_char) free(af->wear_off_char);
        if (af->wear_off_room) free(af->wear_off_room);
        free(af);
        af = next;
    }
}

static void free_mig_player(struct mig_player *p) {
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
        }

        // O_F_SPELLBOOK is outside O_F_UNIQUE block
        if (o_f_flag & O_F_SPELLBOOK) {
            int tmp = mig_getInt(buf);
            for (int i = 0; i < tmp; i++)
                MIG_GET_BYTE(*buf);
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
            // skip backup files
            if (strstr(entry->d_name, ".bak")) continue;
            if (strstr(entry->d_name, ".backup")) continue;
            if (strstr(entry->d_name, ".old")) continue;

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

        // check if insert happened (skip if duplicate)
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
// player pfile migration
// ============================================================================

// parse player status section (stat_vers 47)
static int parse_player_status(char **buf, struct mig_player *p, int stat_vers) {
    char *str;
    int tmp, i;

    // name already read
    str = mig_getString(buf);
    if (str) { strncpy(p->name, str, 63); p->name[63] = 0; free(str); }

    p->pid = mig_getInt(buf);
    p->screen_length = MIG_GET_BYTE(*buf);

    str = mig_getString(buf);
    if (str) { strncpy(p->password, str, 63); p->password[63] = 0; free(str); }

    p->short_descr = mig_getString(buf);
    p->long_descr = mig_getString(buf);
    p->description = mig_getString(buf);
    p->title = mig_getString(buf);

    if (stat_vers < 33) {
        tmp = mig_getInt(buf);
        p->m_class = 1 << (tmp - 1);
        mig_getInt(buf);
    } else {
        p->m_class = (unsigned int)mig_getInt(buf);
    }

    if (stat_vers > 36) {
        p->secondary_class = (unsigned int)mig_getInt(buf);
        // BIT_32 (0x80000000) means "no secondary class" - convert to 0
        if (p->secondary_class == 0x80000000)
            p->secondary_class = 0;
    }
    if (stat_vers > 38)
        p->spec = MIG_GET_BYTE(*buf);

    p->race = MIG_GET_BYTE(*buf);
    p->racewar = MIG_GET_BYTE(*buf);
    p->level = MIG_GET_BYTE(*buf);
    if (stat_vers < 33) MIG_GET_BYTE(*buf);
    p->sex = MIG_GET_BYTE(*buf);
    p->weight = mig_getShort(buf);
    p->height = mig_getShort(buf);
    p->size = MIG_GET_BYTE(*buf);

    p->hometown = mig_getInt(buf);
    p->birthplace = mig_getInt(buf);
    p->orig_birthplace = mig_getInt(buf);

    p->birth_time = mig_getLong(buf);
    p->played_time = mig_getInt(buf);
    p->last_save = mig_getLong(buf);
    p->perm_aging = mig_getShort(buf);

    for (i = 0; i <= MIG_MAX_CIRCLE; i++)
        p->undead_slots[i] = MIG_GET_BYTE(*buf);

    mig_getInt(buf); // last_level, unused

    for (i = 0; i < MIG_NUMB_PC_TIMERS; i++)
        p->timers[i] = mig_getLong(buf);

    // trophy - skip if old format
    if (stat_vers < 45) {
        tmp = MIG_GET_BYTE(*buf);
        for (i = 0; i < tmp; i++) {
            mig_getInt(buf);
            mig_getInt(buf);
        }
    }

    // languages
    int num_langs = mig_getShort(buf);
    for (i = 0; i < num_langs && i < MIG_MAX_TONGUE; i++)
        p->languages[i] = MIG_GET_BYTE(*buf);
    for (; i < num_langs; i++)
        MIG_GET_BYTE(*buf); // skip extra

    // intros
    int num_intros = mig_getShort(buf);
    for (i = 0; i < num_intros && i < MIG_MAX_INTRO; i++) {
        p->intro_pids[i] = mig_getInt(buf);
        p->intro_times[i] = mig_getLong(buf);
    }
    for (; i < num_intros; i++) {
        mig_getInt(buf);
        mig_getLong(buf);
    }

    // forged items
    if (stat_vers > 37 && stat_vers < 40) {
        for (i = 0; i < 100; i++)
            p->forged_items[i] = mig_getInt(buf);
    } else if (stat_vers > 39) {
        for (i = 0; i < MIG_MAX_FORGE_ITEMS; i++)
            p->forged_items[i] = mig_getInt(buf);
    }

    // base stats
    p->str = MIG_GET_BYTE(*buf);
    p->dex = MIG_GET_BYTE(*buf);
    p->agi = MIG_GET_BYTE(*buf);
    p->con = MIG_GET_BYTE(*buf);
    p->pow = MIG_GET_BYTE(*buf);
    p->intel = MIG_GET_BYTE(*buf);
    p->wis = MIG_GET_BYTE(*buf);
    p->cha = MIG_GET_BYTE(*buf);
    p->kar = MIG_GET_BYTE(*buf);
    p->luk = MIG_GET_BYTE(*buf);

    // points
    p->mana = mig_getShort(buf);
    p->base_mana = mig_getShort(buf);
    p->hp = mig_getShort(buf);
    p->spells_memmed = MIG_GET_BYTE(*buf);
    p->base_hp = mig_getShort(buf);
    p->vitality = mig_getShort(buf);
    p->base_vitality = mig_getShort(buf);

    // money
    p->copper = mig_getInt(buf);
    p->silver = mig_getInt(buf);
    p->gold = mig_getInt(buf);
    p->platinum = mig_getInt(buf);

    // experience
    p->exp = mig_getInt(buf);
    mig_getInt(buf); // max_exp unused
    p->epics = mig_getInt(buf);

    if (stat_vers >= 44)
        p->epic_skill_points = mig_getInt(buf);
    if (stat_vers > 46)
        p->skillpoints = mig_getInt(buf);
    if (stat_vers > 40)
        p->spell_bind_used = mig_getInt(buf);
    if (stat_vers < 43)
        mig_getInt(buf); // quaffed_level

    // flags
    p->act = mig_getInt(buf);
    p->act2 = mig_getInt(buf);
    if (stat_vers < 35) {
        mig_getInt(buf);
        mig_getInt(buf);
    }
    p->vote = mig_getInt(buf);
    p->alignment = mig_getInt(buf);
    mig_getInt(buf); // orig_align

    // guild
    p->prestige = mig_getShort(buf);
    p->assoc_id = mig_getShort(buf);
    p->guild_status = mig_getInt(buf);
    p->time_left_guild = mig_getLong(buf);
    p->nb_left_guild = MIG_GET_BYTE(*buf);

    if (stat_vers > 31)
        p->time_unspecced = mig_getLong(buf);

    if (stat_vers <= 35) {
        for (i = 0; i < 5; i++) {
            MIG_GET_BYTE(*buf);
            MIG_GET_BYTE(*buf);
        }
    }

    // frags
    if (stat_vers < 46) {
        mig_getLong(buf);
        mig_getLong(buf);
        p->frags = 0;
        p->oldfrags = 0;
    } else {
        p->frags = mig_getLong(buf);
        p->oldfrags = mig_getLong(buf);
    }

    if (stat_vers < 35) {
        mig_getShort(buf);
        mig_getShort(buf);
    }
    if (stat_vers < 34)
        mig_getInt(buf);
    if (stat_vers < 35)
        mig_getInt(buf);

    // granted commands
    p->num_granted_cmds = mig_getInt(buf);
    if (p->num_granted_cmds > 0) {
        p->granted_cmds = (int *)malloc(p->num_granted_cmds * sizeof(int));
        for (i = 0; i < p->num_granted_cmds; i++)
            p->granted_cmds[i] = mig_getInt(buf);
    }

    // conditions
    for (i = 0; i < MIG_MAX_COND; i++)
        p->conditions[i] = MIG_GET_BYTE(*buf);

    if (stat_vers < 35) {
        for (i = 0; i < 10; i++) // MAX_PETS
            mig_getInt(buf);
    }

    // poof messages
    p->poof_in = mig_getString(buf);
    p->poof_out = mig_getString(buf);
    if (stat_vers > 10) {
        p->poof_in_sound = mig_getString(buf);
        p->poof_out_sound = mig_getString(buf);
    }

    p->echo_toggle = MIG_GET_BYTE(*buf);
    p->prompt = mig_getShort(buf);
    p->wiz_invis = mig_getLong(buf);
    p->law_flags = mig_getLong(buf);
    p->wimpy = mig_getShort(buf);
    p->aggressive = mig_getShort(buf);
    p->highest_level = MIG_GET_BYTE(*buf);

    // bank
    p->bank_copper = mig_getInt(buf);
    p->bank_silver = mig_getInt(buf);
    p->bank_gold = mig_getInt(buf);
    p->bank_platinum = mig_getInt(buf);

    p->numb_deaths = mig_getLong(buf);

    // quest data
    if (stat_vers > 41) {
        p->quest_active = mig_getInt(buf);
        p->quest_mob_vnum = mig_getInt(buf);
        p->quest_type = mig_getInt(buf);
        p->quest_accomplished = mig_getInt(buf);
        p->quest_started = mig_getInt(buf);
        p->quest_zone_number = mig_getInt(buf);
        p->quest_giver = mig_getInt(buf);
        p->quest_level = mig_getInt(buf);
        p->quest_receiver = mig_getInt(buf);
        p->quest_shares_left = mig_getInt(buf);
        p->quest_kill_how_many = mig_getInt(buf);
        p->quest_kill_original = mig_getInt(buf);
        p->quest_map_room = mig_getInt(buf);
        p->quest_map_bought = mig_getInt(buf);
    }

    return 1;
}

// parse player skills section
static int parse_player_skills(char **buf, struct mig_player *p) {
    int skill_vers = MIG_GET_BYTE(*buf);
    if (skill_vers > SAV_SKILLVERS) return 0;

    int n = mig_getInt(buf);
    for (int i = 0; i < MIG_MAX_SKILLS; i++) {
        if (i < n) {
            p->skills_learned[i] = MIG_GET_BYTE(*buf);
            p->skills_taught[i] = MIG_GET_BYTE(*buf);
            MIG_GET_BYTE(*buf); // unused
        }
    }
    // skip extra skills if file has more than we support
    for (int i = MIG_MAX_SKILLS; i < n; i++) {
        MIG_GET_BYTE(*buf);
        MIG_GET_BYTE(*buf);
        MIG_GET_BYTE(*buf);
    }

    // skill usage loop
    int tmp;
    do { tmp = mig_getInt(buf); } while (tmp != 0);

    // skill usages
    n = mig_getShort(buf);
    for (int i = 0; i < n; i++) {
        mig_getLong(buf);
        MIG_GET_BYTE(*buf);
    }

    return 1;
}

// parse player witness section
static int parse_player_witness(char **buf) {
    int witness_vers = MIG_GET_BYTE(*buf);
    if (witness_vers > SAV_WTNSVERS) return 0;

    int count = mig_getInt(buf);
    // we skip witnesses for migration - they'll be regenerated
    for (int i = 0; i < count; i++) {
        mig_getInt(buf);  // crime
        mig_getInt(buf);  // room
        mig_getString(buf); // attacker - free it
        mig_getString(buf); // victim - free it
        mig_getLong(buf); // time
    }
    return 1;
}

// parse player affects section
static int parse_player_affects(char **buf, struct mig_player *p) {
    int aff_vers = MIG_GET_BYTE(*buf);
    if (aff_vers > SAV_AFFVERS) return 0;

    int count = mig_getShort(buf);
    struct mig_affect *last = NULL;

    for (int i = 0; i < count; i++) {
        struct mig_affect *af = (struct mig_affect *)malloc(sizeof(struct mig_affect));
        memset(af, 0, sizeof(struct mig_affect));

        if (aff_vers > 4) {
            if (aff_vers > 5) {
                unsigned char custom_msgs = MIG_GET_BYTE(*buf);
                if (custom_msgs & 1)
                    af->wear_off_char = mig_getString(buf);
                if (custom_msgs & 2)
                    af->wear_off_room = mig_getString(buf);
                af->type = mig_getShort(buf);
            } else {
                af->type = mig_getInt(buf);
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
            mig_getLong(buf); // bitvector6 unused

            if (aff_vers > 7)
                af->level = mig_getShort(buf);
            else
                af->level = p->level;
        } else {
            af->type = mig_getInt(buf);
            af->duration = mig_getShort(buf);
            af->modifier = mig_getInt(buf);
            af->location = MIG_GET_BYTE(*buf);
            mig_getInt(buf); // loc2
            af->bitvector1 = mig_getLong(buf);
            af->bitvector2 = mig_getLong(buf);
            af->bitvector3 = mig_getLong(buf);
            af->bitvector4 = mig_getLong(buf);
            af->bitvector5 = mig_getLong(buf);
            af->flags = 0;
            if (aff_vers == 4) {
                mig_getLong(buf); // bitvector6
                long short_dur = mig_getLong(buf);
                if (short_dur > 0) {
                    af->flags = 1; // AFFTYPE_SHORT
                    af->duration = short_dur;
                }
            }
            af->level = p->level;
        }

        af->next = NULL;
        if (last)
            last->next = af;
        else
            p->affects = af;
        last = af;
    }

    return 1;
}

// parse player items section
static int parse_player_items(char **buf, struct mig_player *p) {
    char *start = *buf;
    int obj_vers = MIG_GET_BYTE(*buf);
    if (obj_vers > SAV_ITEMVERS) return 0;

    int total_count = mig_getInt(buf);
    struct mig_obj *container_stack[32];
    int stack_depth = 0;
    struct mig_obj *last_inventory = NULL;

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
        mig_getShort(buf); // craftsmanship
        mig_getShort(buf); // condition

        int wear_slot = -1;
        if (o_f_flag & O_F_WORN)
            wear_slot = MIG_GET_BYTE(*buf);

        if (o_f_flag & O_F_COUNT)
            mig_getShort(buf); // quantity

        if (o_f_flag & O_F_AFFECTS) {
            int aff_count = MIG_GET_BYTE(*buf);
            while (aff_count--) {
                mig_getInt(buf);
                mig_getShort(buf);
                mig_getShort(buf);
                mig_getInt(buf);
            }
        }

        if (o_f_flag & O_F_UNIQUE) {
            unsigned long o_u_flag = mig_getInt(buf);

            if (o_u_flag & O_U_KEYS) obj->name = mig_getString(buf);
            if (o_u_flag & O_U_DESC1) obj->description = mig_getString(buf);
            if (o_u_flag & O_U_DESC2) obj->short_descr = mig_getString(buf);
            if (o_u_flag & O_U_DESC3) obj->action_descr = mig_getString(buf);
            if (o_u_flag & O_U_EDESC) {
                int nDescs = mig_getShort(buf);
                while (nDescs--) {
                    char *kw = mig_getString(buf);
                    char *desc = mig_getString(buf);
                    if (kw) free(kw);
                    if (desc) free(desc);
                }
            }
            if (o_u_flag & O_U_VAL0) obj->value[0] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL1) obj->value[1] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL2) obj->value[2] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL3) obj->value[3] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL4) obj->value[4] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL5) obj->value[5] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL6) obj->value[6] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL7) obj->value[7] = mig_getInt(buf);
            if (o_u_flag & O_U_TIMER) {
                obj->timer = mig_getInt(buf);
                mig_getInt(buf); mig_getInt(buf); mig_getInt(buf);
            }
            if (o_u_flag & O_U_TRAP) {
                mig_getShort(buf); mig_getShort(buf);
                mig_getShort(buf); mig_getShort(buf);
            }
            if (o_u_flag & O_U_TYPE) MIG_GET_BYTE(*buf);
            if (o_u_flag & O_U_WEAR) mig_getInt(buf);
            if (o_u_flag & O_U_EXTRA) obj->extra_flags = mig_getInt(buf);
            if (o_u_flag & O_U_ANTI) mig_getInt(buf);
            if (o_u_flag & O_U_ANTI2) mig_getInt(buf);
            if (o_u_flag & O_U_EXTRA2) mig_getInt(buf);
            if (o_u_flag & O_U_WEIGHT) obj->weight = mig_getInt(buf);
            if (o_u_flag & O_U_MATERIAL) MIG_GET_BYTE(*buf);
            if (o_u_flag & O_U_COST) obj->cost = mig_getInt(buf);
            if (o_u_flag & O_U_BV1) mig_getLong(buf);
            if (o_u_flag & O_U_BV2) mig_getLong(buf);
            if (o_u_flag & O_U_BV3) mig_getLong(buf);
            if (o_u_flag & O_U_BV4) mig_getLong(buf);
            if (o_u_flag & O_U_BV5) mig_getLong(buf);
            if (o_u_flag & O_U_AFFS) {
                for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                    MIG_GET_BYTE(*buf);
                    MIG_GET_BYTE(*buf);
                }
            }
        }

        // O_F_SPELLBOOK is outside O_F_UNIQUE block
        if (o_f_flag & O_F_SPELLBOOK) {
            int tmp = mig_getInt(buf);
            for (int i = 0; i < tmp; i++)
                MIG_GET_BYTE(*buf);
        }

        // link to appropriate place
        if (stack_depth > 0) {
            struct mig_obj *parent = container_stack[stack_depth - 1];
            if (!parent->contains) {
                parent->contains = obj;
            } else {
                struct mig_obj *c = parent->contains;
                while (c->next) c = c->next;
                c->next = obj;
            }
        } else if (wear_slot >= 0 && wear_slot < MIG_MAX_WEAR) {
            p->equipment[wear_slot] = obj;
        } else {
            if (!p->inventory)
                p->inventory = obj;
            if (last_inventory)
                last_inventory->next = obj;
            last_inventory = obj;
        }

        if (o_f_flag & O_F_CONTAINS) {
            container_stack[stack_depth++] = obj;
        }
    }

    return 1;
}

// check if player already exists in database
static int player_exists_in_db(const char *name) {
    char *esc_name = sql_escape_string(name);
    if (!esc_name) return 0;

    char query[256];
    snprintf(query, sizeof(query),
        "SELECT COUNT(*) FROM player_data WHERE LOWER(name) = LOWER('%s')", esc_name);
    free(esc_name);

    MYSQL_RES *result = db_query("%s", query);
    if (!result) return 0;

    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? atoi(row[0]) : 0;
    mysql_free_result(result);

    return count > 0;
}

// save player item to database, returns item_id
static int save_player_item(int pid, struct mig_obj *obj, int equip_slot, int container_id) {
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
        "INSERT INTO player_items ("
        "pid, vnum, equip_slot, container_id, quantity, "
        "weight, cost, timer, extra_flags, "
        "value0, value1, value2, value3, value4, value5, value6, value7, "
        "name, short_descr, description, action_descr"
        ") VALUES ("
        "%d, %d, %d, %s, 1, "
        "%d, %d, %ld, %lu, "
        "%d, %d, %d, %d, %d, %d, %d, %d, "
        "%s, %s, %s, %s"
        ")",
        pid, obj->vnum, equip_slot, container_str,
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

    // save contained items
    for (struct mig_obj *c = obj->contains; c; c = c->next) {
        save_player_item(pid, c, 0, item_id);
    }

    return item_id;
}

// save complete player to database
static int save_player_to_db(struct mig_player *p) {
    if (!p || !p->name[0]) return 0;

    char *esc_name = sql_escape_string(p->name);
    char *esc_short = p->short_descr ? sql_escape_string(p->short_descr) : NULL;
    char *esc_long = p->long_descr ? sql_escape_string(p->long_descr) : NULL;
    char *esc_desc = p->description ? sql_escape_string(p->description) : NULL;
    char *esc_title = p->title ? sql_escape_string(p->title) : NULL;
    char *esc_poof_in = p->poof_in ? sql_escape_string(p->poof_in) : NULL;
    char *esc_poof_out = p->poof_out ? sql_escape_string(p->poof_out) : NULL;
    char *esc_poof_in_snd = p->poof_in_sound ? sql_escape_string(p->poof_in_sound) : NULL;
    char *esc_poof_out_snd = p->poof_out_sound ? sql_escape_string(p->poof_out_sound) : NULL;

    // build main insert query
    char query[16384];
    snprintf(query, sizeof(query),
        "INSERT INTO player_data ("
        "name, short_descr, long_descr, description, title, "
        "m_class, secondary_class, spec, race, racewar, level, sex, "
        "weight, height, size, hometown, birthplace, orig_birthplace, "
        "birth_time, played_time, last_save, perm_aging, "
        "base_str, base_dex, base_agi, base_con, base_pow, base_int, base_wis, base_cha, base_kar, base_luk, "
        "mana, base_mana, hit_diff, base_hit, vitality, base_vitality, spells_memmed_extra, "
        "copper, silver, gold, platinum, bank_copper, bank_silver, bank_gold, bank_platinum, "
        "exp, epics, epic_skill_points, skillpoints, spell_bind_used, "
        "act, act2, vote, alignment, prestige, assoc_id, guild_status, "
        "time_left_guild, nb_left_guild, time_unspecced, frags, oldfrags, numb_deaths, "
        "condition_0, condition_1, condition_2, condition_3, condition_4, "
        "poof_in, poof_out, poof_in_sound, poof_out_sound, "
        "echo_toggle, prompt, wiz_invis, law_flags, wimpy, aggressive, highest_level, screen_length, "
        "quest_active, quest_mob_vnum, quest_type, quest_accomplished, quest_started, "
        "quest_zone_number, quest_giver, quest_level, quest_receiver, "
        "quest_shares_left, quest_kill_how_many, quest_kill_original, quest_map_room, quest_map_bought"
        ") VALUES ("
        "'%s', %s%s%s, %s%s%s, %s%s%s, %s%s%s, "
        "%u, %u, %d, %d, %d, %d, %d, "
        "%d, %d, %d, %d, %d, %d, "
        "%ld, %ld, %ld, %d, "
        "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
        "%d, %d, %d, %d, %d, %d, %d, "
        "%d, %d, %d, %d, %d, %d, %d, %d, "
        "%d, %d, %d, %d, %d, "
        "%u, %u, %d, %d, %d, %d, %d, "
        "%ld, %d, %ld, %ld, %ld, %ld, "
        "%d, %d, %d, %d, %d, "
        "%s%s%s, %s%s%s, %s%s%s, %s%s%s, "
        "%d, %d, %ld, %lu, %d, %d, %d, %d, "
        "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d"
        ")",
        esc_name,
        esc_short ? "'" : "", esc_short ? esc_short : "NULL", esc_short ? "'" : "",
        esc_long ? "'" : "", esc_long ? esc_long : "NULL", esc_long ? "'" : "",
        esc_desc ? "'" : "", esc_desc ? esc_desc : "NULL", esc_desc ? "'" : "",
        esc_title ? "'" : "", esc_title ? esc_title : "NULL", esc_title ? "'" : "",
        p->m_class, p->secondary_class, p->spec, p->race, p->racewar, p->level, p->sex,
        p->weight, p->height, p->size, p->hometown, p->birthplace, p->orig_birthplace,
        p->birth_time, p->played_time, p->last_save, p->perm_aging,
        p->str, p->dex, p->agi, p->con, p->pow, p->intel, p->wis, p->cha, p->kar, p->luk,
        p->mana, p->base_mana, p->hp - p->base_hp, p->base_hp, p->vitality, p->base_vitality, p->spells_memmed,
        p->copper, p->silver, p->gold, p->platinum,
        p->bank_copper, p->bank_silver, p->bank_gold, p->bank_platinum,
        p->exp, p->epics, p->epic_skill_points, p->skillpoints, p->spell_bind_used,
        p->act, p->act2, p->vote, p->alignment, p->prestige, p->assoc_id, p->guild_status,
        p->time_left_guild, p->nb_left_guild, p->time_unspecced, p->frags, p->oldfrags, p->numb_deaths,
        p->conditions[0], p->conditions[1], p->conditions[2], p->conditions[3], p->conditions[4],
        esc_poof_in ? "'" : "", esc_poof_in ? esc_poof_in : "NULL", esc_poof_in ? "'" : "",
        esc_poof_out ? "'" : "", esc_poof_out ? esc_poof_out : "NULL", esc_poof_out ? "'" : "",
        esc_poof_in_snd ? "'" : "", esc_poof_in_snd ? esc_poof_in_snd : "NULL", esc_poof_in_snd ? "'" : "",
        esc_poof_out_snd ? "'" : "", esc_poof_out_snd ? esc_poof_out_snd : "NULL", esc_poof_out_snd ? "'" : "",
        p->echo_toggle, p->prompt, p->wiz_invis, p->law_flags, p->wimpy, (short)p->aggressive, p->highest_level, p->screen_length,
        p->quest_active, p->quest_mob_vnum, p->quest_type, p->quest_accomplished, p->quest_started,
        p->quest_zone_number, p->quest_giver, p->quest_level, p->quest_receiver,
        p->quest_shares_left, p->quest_kill_how_many, p->quest_kill_original, p->quest_map_room, p->quest_map_bought);

    if (esc_name) free(esc_name);
    if (esc_short) free(esc_short);
    if (esc_long) free(esc_long);
    if (esc_desc) free(esc_desc);
    if (esc_title) free(esc_title);
    if (esc_poof_in) free(esc_poof_in);
    if (esc_poof_out) free(esc_poof_out);
    if (esc_poof_in_snd) free(esc_poof_in_snd);
    if (esc_poof_out_snd) free(esc_poof_out_snd);

    // check if player already exists
    char pid_query[256];
    snprintf(pid_query, sizeof(pid_query), "SELECT pid FROM player_data WHERE name = '%s'", p->name);
    MYSQL_RES *check = db_query(pid_query);
    int existing_pid = 0;
    if (check) {
        MYSQL_ROW r = mysql_fetch_row(check);
        if (r && r[0]) existing_pid = atoi(r[0]);
        mysql_free_result(check);
    }

    if (existing_pid > 0) {
        // player exists - delete old data and re-insert fresh
        qry("DELETE FROM player_skills WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_languages WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_timers WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_undead_slots WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_forged_items WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_intros WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_granted_cmds WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_affects WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_items WHERE pid = %d", existing_pid);
        qry("DELETE FROM player_data WHERE pid = %d", existing_pid);
    }

    if (!qry("%s", query))
        return 0;

    // get pid (either new insert or lookup)
    MYSQL_RES *result = db_query(pid_query);
    if (!result) return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    int pid = row ? atoi(row[0]) : 0;
    mysql_free_result(result);

    if (pid <= 0) return 0;

    // save skills (non-zero only)
    for (int i = 0; i < MIG_MAX_SKILLS; i++) {
        if (p->skills_learned[i] > 0 || p->skills_taught[i] > 0) {
            qry("INSERT INTO player_skills (pid, skill_id, learned, taught) VALUES (%d, %d, %d, %d) "
                "ON DUPLICATE KEY UPDATE learned=%d, taught=%d",
                pid, i, p->skills_learned[i], p->skills_taught[i],
                p->skills_learned[i], p->skills_taught[i]);
        }
    }

    // save languages
    for (int i = 0; i < MIG_MAX_TONGUE; i++) {
        if (p->languages[i] > 0) {
            qry("INSERT INTO player_languages (pid, tongue_id, proficiency) VALUES (%d, %d, %d) "
                "ON DUPLICATE KEY UPDATE proficiency=%d",
                pid, i, p->languages[i], p->languages[i]);
        }
    }

    // save timers
    for (int i = 0; i < MIG_NUMB_PC_TIMERS; i++) {
        if (p->timers[i] != 0) {
            qry("INSERT INTO player_timers (pid, timer_id, timer_value) VALUES (%d, %d, %ld) "
                "ON DUPLICATE KEY UPDATE timer_value=%ld",
                pid, i, p->timers[i], p->timers[i]);
        }
    }

    // save undead slots
    for (int i = 0; i <= MIG_MAX_CIRCLE; i++) {
        if (p->undead_slots[i] > 0) {
            qry("INSERT INTO player_undead_slots (pid, circle, slots) VALUES (%d, %d, %d) "
                "ON DUPLICATE KEY UPDATE slots=%d",
                pid, i, p->undead_slots[i], p->undead_slots[i]);
        }
    }

    // save forged items
    for (int i = 0; i < MIG_MAX_FORGE_ITEMS; i++) {
        if (p->forged_items[i] != 0) {
            qry("INSERT INTO player_forged_items (pid, forge_index, item_vnum) VALUES (%d, %d, %d) "
                "ON DUPLICATE KEY UPDATE item_vnum=%d",
                pid, i, p->forged_items[i], p->forged_items[i]);
        }
    }

    // save intros
    for (int i = 0; i < MIG_MAX_INTRO; i++) {
        if (p->intro_pids[i] != 0) {
            qry("INSERT INTO player_intros (pid, intro_index, intro_pid, intro_time) VALUES (%d, %d, %d, 0) "
                "ON DUPLICATE KEY UPDATE intro_pid=%d",
                pid, i, p->intro_pids[i], p->intro_pids[i]);
        }
    }

    // save granted commands
    for (int i = 0; i < p->num_granted_cmds; i++) {
        qry("INSERT INTO player_granted_cmds (pid, cmd_num) VALUES (%d, %d) "
            "ON DUPLICATE KEY UPDATE cmd_num=cmd_num",
            pid, p->granted_cmds[i]);
    }

    // save affects
    for (struct mig_affect *af = p->affects; af; af = af->next) {
        char *esc_woc = af->wear_off_char ? sql_escape_string(af->wear_off_char) : NULL;
        char *esc_wor = af->wear_off_room ? sql_escape_string(af->wear_off_room) : NULL;
        qry("INSERT INTO player_affects (pid, type, duration, flags, modifier, location, level, "
            "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5, custom_msg_char, custom_msg_room) "
            "VALUES (%d, %d, %d, %d, %d, %d, %d, %ld, %ld, %ld, %ld, %ld, %s, %s)",
            pid, af->type, af->duration, af->flags, af->modifier, af->location, af->level,
            af->bitvector1, af->bitvector2, af->bitvector3, af->bitvector4, af->bitvector5,
            esc_woc ? esc_woc : "NULL", esc_wor ? esc_wor : "NULL");
        if (esc_woc) free(esc_woc);
        if (esc_wor) free(esc_wor);
    }

    // save equipment
    for (int i = 0; i < MIG_MAX_WEAR; i++) {
        if (p->equipment[i]) {
            save_player_item(pid, p->equipment[i], i + 1, 0);
        }
    }

    // save inventory
    for (struct mig_obj *obj = p->inventory; obj; obj = obj->next) {
        save_player_item(pid, obj, 0, 0);
    }

    return pid;
}

// parse complete pfile
static struct mig_player *parse_player_pfile(const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;

    char buffer[250000];
    int size = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);

    if (size < 50) return NULL;

    char *buf = buffer;

    // check pfile header
    int save_vers = MIG_GET_BYTE(buf);
    if (save_vers != SAV_SAVEVERS) {
        printf("  wrong save version: %d\n", save_vers);
        return NULL;
    }

    int ss = MIG_GET_BYTE(buf);
    int is = MIG_GET_BYTE(buf);
    int ls = MIG_GET_BYTE(buf);
    if (ss != 2 || is != 4 || ls != 8) {
        printf("  wrong machine format: %d/%d/%d\n", ss, is, ls);
        return NULL;
    }

    MIG_GET_BYTE(buf); // rent type

    // read offsets
    int skill_off = mig_getInt(&buf);
    int witness_off = mig_getInt(&buf);
    int affect_off = mig_getInt(&buf);
    int item_off = mig_getInt(&buf);
    int size_off = mig_getInt(&buf);
    mig_getInt(&buf); // act3/surname
    mig_getInt(&buf); // room
    mig_getLong(&buf); // save time

    // validate offsets
    if (skill_off >= size || witness_off >= size || affect_off >= size ||
        item_off >= size || size_off > size) {
        printf("  invalid offsets: skill=%d witness=%d affect=%d item=%d size=%d (filesize=%d)\n",
               skill_off, witness_off, affect_off, item_off, size_off, size);
        return NULL;
    }

    // debug: check position
    int pos = (int)(buf - buffer);

    // allocate player
    struct mig_player *p = (struct mig_player *)malloc(sizeof(struct mig_player));
    memset(p, 0, sizeof(struct mig_player));

    // parse status
    int stat_vers = MIG_GET_BYTE(buf);
    if (stat_vers > SAV_STATVERS) {
        printf("  wrong stat version: %d at pos %d\n", stat_vers, pos);
        free(p);
        return NULL;
    }

    if (!parse_player_status(&buf, p, stat_vers)) {
        free_mig_player(p);
        return NULL;
    }

    // parse skills
    buf = buffer + skill_off;
    if (!parse_player_skills(&buf, p)) {
        free_mig_player(p);
        return NULL;
    }

    // parse witness (skip)
    buf = buffer + witness_off;
    parse_player_witness(&buf);

    // parse affects
    buf = buffer + affect_off;
    if (!parse_player_affects(&buf, p)) {
        free_mig_player(p);
        return NULL;
    }

    // parse items
    buf = buffer + item_off;
    if (item_off >= size || item_off < 0) {
        printf("  invalid item_off: %d (size: %d)\n", item_off, size);
        free_mig_player(p);
        return NULL;
    }
    if (!parse_player_items(&buf, p)) {
        free_mig_player(p);
        return NULL;
    }

    return p;
}

// migrate all player pfiles
static int migrate_players_from_files(void) {
    int count = 0;
    int errors = 0;

    printf("migrating players from pfiles...\n");

    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Players/%c", letter);

        DIR *dir = opendir(dirname);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (strstr(entry->d_name, ".bak")) continue;
            if (strstr(entry->d_name, ".locker")) continue;
            if (strstr(entry->d_name, ".spellbook")) continue;
            if (strstr(entry->d_name, ".preconvert")) continue;
            if (strstr(entry->d_name, ".old")) continue;
            if (strstr(entry->d_name, ".backup")) continue;

            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);

            struct stat st;
            if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode)) continue;

            struct mig_player *p = parse_player_pfile(filepath);
            if (!p) {
                printf("  error parsing: %s\n", filepath);
                errors++;
                continue;
            }

            int pid = save_player_to_db(p);
            if (pid > 0) {
                printf("  migrated: %s (level %d, pid %d)\n", p->name, p->level, pid);
                count++;
            } else {
                printf("  error saving: %s\n", p->name);
                errors++;
            }

            free_mig_player(p);
        }

        closedir(dir);
    }

    printf("players migration complete: %d migrated, %d errors\n", count, errors);
    return count;
}

// ============================================================================
// locker migration
// ============================================================================

// save locker item to database, returns item_id
static int save_locker_item(int locker_id, struct mig_obj *obj, int container_id) {
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

// parse locker items from binary buffer (same format as player items, but no equipment slots)
static struct mig_obj *parse_locker_items(char **buf) {
    int obj_vers = MIG_GET_BYTE(*buf);
    if (obj_vers > SAV_ITEMVERS) {
        printf("  error: locker item version %d > %d\n", obj_vers, SAV_ITEMVERS);
        return NULL;
    }

    mig_getInt(buf); // total count - not needed

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
        mig_getShort(buf); // craftsmanship
        mig_getShort(buf); // condition

        if (o_f_flag & O_F_WORN)
            MIG_GET_BYTE(*buf); // wear location - skip for lockers

        if (o_f_flag & O_F_COUNT)
            mig_getShort(buf); // quantity

        if (o_f_flag & O_F_AFFECTS) {
            int aff_count = MIG_GET_BYTE(*buf);
            while (aff_count--) {
                mig_getInt(buf);
                mig_getShort(buf);
                mig_getShort(buf);
                mig_getInt(buf);
            }
        }

        if (o_f_flag & O_F_UNIQUE) {
            unsigned long o_u_flag = mig_getInt(buf);

            if (o_u_flag & O_U_KEYS) obj->name = mig_getString(buf);
            if (o_u_flag & O_U_DESC1) obj->description = mig_getString(buf);
            if (o_u_flag & O_U_DESC2) obj->short_descr = mig_getString(buf);
            if (o_u_flag & O_U_DESC3) obj->action_descr = mig_getString(buf);
            if (o_u_flag & O_U_EDESC) {
                int nDescs = mig_getShort(buf);
                while (nDescs--) {
                    char *kw = mig_getString(buf);
                    char *desc = mig_getString(buf);
                    if (kw) free(kw);
                    if (desc) free(desc);
                }
            }
            if (o_u_flag & O_U_VAL0) obj->value[0] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL1) obj->value[1] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL2) obj->value[2] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL3) obj->value[3] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL4) obj->value[4] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL5) obj->value[5] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL6) obj->value[6] = mig_getInt(buf);
            if (o_u_flag & O_U_VAL7) obj->value[7] = mig_getInt(buf);
            if (o_u_flag & O_U_TIMER) {
                obj->timer = mig_getInt(buf);
                mig_getInt(buf); mig_getInt(buf); mig_getInt(buf);
            }
            if (o_u_flag & O_U_TRAP) {
                mig_getShort(buf); mig_getShort(buf);
                mig_getShort(buf); mig_getShort(buf);
            }
            if (o_u_flag & O_U_TYPE) MIG_GET_BYTE(*buf);
            if (o_u_flag & O_U_WEAR) mig_getInt(buf);
            if (o_u_flag & O_U_EXTRA) obj->extra_flags = mig_getInt(buf);
            if (o_u_flag & O_U_ANTI) mig_getInt(buf);
            if (o_u_flag & O_U_ANTI2) mig_getInt(buf);
            if (o_u_flag & O_U_EXTRA2) mig_getInt(buf);
            if (o_u_flag & O_U_WEIGHT) obj->weight = mig_getInt(buf);
            if (o_u_flag & O_U_MATERIAL) MIG_GET_BYTE(*buf);
            if (o_u_flag & O_U_COST) obj->cost = mig_getInt(buf);
            if (o_u_flag & O_U_BV1) mig_getLong(buf);
            if (o_u_flag & O_U_BV2) mig_getLong(buf);
            if (o_u_flag & O_U_BV3) mig_getLong(buf);
            if (o_u_flag & O_U_BV4) mig_getLong(buf);
            if (o_u_flag & O_U_BV5) mig_getLong(buf);
            if (o_u_flag & O_U_AFFS) {
                for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                    MIG_GET_BYTE(*buf);
                    MIG_GET_BYTE(*buf);
                }
            }
        }

        if (o_f_flag & O_F_SPELLBOOK) {
            int tmp = mig_getInt(buf);
            for (int i = 0; i < tmp; i++)
                MIG_GET_BYTE(*buf);
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
            if (!root) root = obj;
            if (last) last->next = obj;
            last = obj;
        }

        if (o_f_flag & O_F_CONTAINS) {
            container_stack[stack_depth++] = obj;
        }
    }

    return root;
}

// migrate a single locker file
static int migrate_locker_file(const char *filepath, const char *locker_name) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return 0;

    char buffer[250000];
    int size = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);

    if (size < 50) return 0;

    char *buf = buffer;

    // check pfile header (same format as player files)
    int save_vers = MIG_GET_BYTE(buf);
    if (save_vers != SAV_SAVEVERS) {
        printf("  wrong locker version: %d\n", save_vers);
        return 0;
    }

    int ss = MIG_GET_BYTE(buf);
    int is = MIG_GET_BYTE(buf);
    int ls = MIG_GET_BYTE(buf);
    if (ss != 2 || is != 4 || ls != 8) {
        printf("  wrong locker machine format: %d/%d/%d\n", ss, is, ls);
        return 0;
    }

    MIG_GET_BYTE(buf); // rent type

    // read offsets
    int skill_off = mig_getInt(&buf);
    int witness_off = mig_getInt(&buf);
    int affect_off = mig_getInt(&buf);
    int item_off = mig_getInt(&buf);
    int size_off = mig_getInt(&buf);
    mig_getInt(&buf); // act3/surname
    mig_getInt(&buf); // room
    mig_getLong(&buf); // save time

    // validate item offset
    if (item_off < 0 || item_off >= size) {
        printf("  invalid locker item_off: %d (size: %d)\n", item_off, size);
        return 0;
    }

    // parse status section to get race/racewar
    int stat_vers = MIG_GET_BYTE(buf);
    if (stat_vers > SAV_STATVERS) {
        printf("  wrong locker stat version: %d\n", stat_vers);
        return 0;
    }

    // we need to read some fields from status to get race/racewar
    // based on restoreStatus in files.c, race is at a specific offset
    // let's read the status section fields we need

    // name (skip - we use locker_name)
    mig_getString(&buf);
    // short_descr
    if (stat_vers >= 32) mig_getString(&buf);
    else { mig_getString(&buf); mig_getString(&buf); mig_getString(&buf); }
    // password (skip)
    mig_getString(&buf);
    // title
    mig_getString(&buf);

    // m_class
    mig_getInt(&buf);
    // secondary_class (int if stat_vers > 36)
    if (stat_vers > 36) mig_getInt(&buf);
    // spec (byte if stat_vers > 38)
    if (stat_vers > 38) MIG_GET_BYTE(buf);
    // race (byte)
    int race = MIG_GET_BYTE(buf);
    // racewar (byte)
    int racewar = MIG_GET_BYTE(buf);

    // determine owner from locker name
    int owner_pid = 0;
    int owner_assoc_id = 0;

    // guild locker: "guild.X.locker" where X is assoc_id
    if (strncasecmp(locker_name, "guild.", 6) == 0) {
        // extract assoc_id
        const char *p = locker_name + 6;
        owner_assoc_id = atoi(p);
    } else {
        // player locker: "playername.locker" - extract player name
        char pname[128];
        strncpy(pname, locker_name, sizeof(pname) - 1);
        pname[sizeof(pname) - 1] = '\0';
        char *dot = strstr(pname, ".locker");
        if (dot) *dot = '\0';

        // look up player pid
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
    buf = buffer + item_off;
    struct mig_obj *items = parse_locker_items(&buf);
    int item_count = 0;
    for (struct mig_obj *obj = items; obj; obj = obj->next) {
        save_locker_item(locker_id, obj, 0);
        item_count++;
    }

    // free items (free_mig_obj handles next recursively)
    free_mig_obj(items);

    return item_count;
}

// migrate all locker files
static int migrate_lockers_from_files(void) {
    int count = 0;
    int errors = 0;
    int total_items = 0;

    printf("migrating lockers from pfiles...\n");

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
                printf("  migrated: %s (%d items)\n", entry->d_name, items);
                count++;
                total_items += items;
            } else {
                printf("  error: %s\n", entry->d_name);
                errors++;
            }
        }
        closedir(dir);
    }

    printf("lockers migration complete: %d migrated, %d items, %d errors\n", count, total_items, errors);
    return count;
}

// ============================================================================
// spellbook migration
// ============================================================================

static int migrate_spellbooks_from_files(void) {
    printf("=== migrating spellbooks ===\n");

    int count = 0;
    int errors = 0;
    int total_mobs = 0;

    // iterate Players/[a-z]/*.spellbook
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirpath[256];
        snprintf(dirpath, sizeof(dirpath), "Players/%c", letter);

        DIR *dir = opendir(dirpath);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir))) {
            // check for .spellbook extension
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
                printf("  warning: player '%s' not found in database\n", player_name);
                errors++;
                continue;
            }
            MYSQL_ROW row = mysql_fetch_row(result);
            if (!row || !row[0]) {
                mysql_free_result(result);
                printf("  warning: player '%s' not found in database\n", player_name);
                errors++;
                continue;
            }
            int pid = atoi(row[0]);
            mysql_free_result(result);

            // read spellbook file
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
            FILE *f = fopen(filepath, "r");
            if (!f) {
                printf("  error opening: %s\n", filepath);
                errors++;
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

            printf("  migrated: %s (%d mobs)\n", player_name, mob_count);
            count++;
        }
        closedir(dir);
    }

    printf("spellbooks: %d migrated, %d mobs total, %d errors\n", count, total_mobs, errors);
    return count;
}

// ============================================================================
// main
// ============================================================================

int main(int argc, char **argv) {
    printf("=== pfile migration tool ===\n\n");

    // load env file - required
    FILE *env = fopen(".env", "r");
    if (!env) {
        printf("error: .env file not found\n");
        return 1;
    }

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

    // check required env vars
    const char *db_host = getenv("DB_HOST");
    const char *db_user = getenv("DB_USER");
    const char *db_passwd = getenv("DB_PASSWD");
    const char *db_name = getenv("DB_NAME");

    if (!db_host || !db_host[0]) {
        printf("error: DB_HOST not set in .env\n");
        return 1;
    }
    if (!db_user || !db_user[0]) {
        printf("error: DB_USER not set in .env\n");
        return 1;
    }
    if (!db_passwd || !db_passwd[0]) {
        printf("error: DB_PASSWD not set in .env\n");
        return 1;
    }
    if (!db_name || !db_name[0]) {
        printf("error: DB_NAME not set in .env\n");
        return 1;
    }

    printf("loaded .env file\n");
    printf("connecting to database: %s@%s/%s\n", db_user, db_host, db_name);

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
    printf("\n");
    int players = migrate_players_from_files();
    printf("\n");
    int lockers = migrate_lockers_from_files();
    printf("\n");
    int spellbooks = migrate_spellbooks_from_files();

    printf("\n=== migration complete ===\n");
    printf("accounts: %d\n", accounts);
    printf("recipes: %d\n", recipes);
    printf("ships: %d\n", ships);
    printf("guilds: %d\n", guilds);
    printf("corpses: %d\n", corpses);
    printf("saved items: %d\n", saved_items);
    printf("players: %d\n", players);
    printf("lockers: %d\n", lockers);
    printf("spellbooks: %d\n", spellbooks);

    return 0;
}
