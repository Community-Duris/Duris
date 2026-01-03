// migrate_common.h
// shared structs and declarations for pfile migration tool

#ifndef MIGRATE_COMMON_H
#define MIGRATE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <mysql.h>

#include "../src/structs.h"
#include "../src/utils.h"
#include "../src/sql.h"
#include "../src/sql_player.h"
#include "../src/account.h"
#include "../src/assocs.h"
#include "../src/ships/ships.h"

// constants from config.h
#define MAX_OBJ_AFFECT 4

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

// binary read macro
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
    int hometown, birthplace, orig_birthplace, last_room;
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
    unsigned int act, act2, act3;
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

// external vars from migrate_stubs.c
extern ush_int mig_getShort(char **buf);
extern uint mig_getInt(char **buf);
extern long mig_getLong(char **buf);
extern char *mig_getString(char **buf);
extern MYSQL *DB;

// forward declarations from migrate_stubs.c
extern char *str_dup(const char *source);
extern void __free(void *p, char *file, int line);
extern char *sql_escape_string(const char *str);
extern MYSQL_RES *db_query(const char *format, ...);
extern bool qry(const char *format, ...);

// global vars
extern P_Guild guild_list;
extern char buf[MAX_STRING_LENGTH];

// progress bar
struct progress_bar {
    int total;
    int current;
    struct timeval start_time;
    const char *prefix;
};

void progress_init(struct progress_bar *pb, int total, const char *prefix);
void progress_update(struct progress_bar *pb, int current);
void progress_finish(struct progress_bar *pb);

// migrate_common.c
void logit(int type, const char *format, ...);
void debug(const char *format, ...);
char *fread_string(FILE *fl);
void free_mig_obj(struct mig_obj *obj);
void free_mig_affect(struct mig_affect *af);
void free_mig_player(struct mig_player *p);

// migrate_objects.c
struct mig_obj *parse_binary_objects(char **buf);
struct mig_obj *parse_locker_items(char **buf);
int parse_player_items(char **buf, struct mig_player *p);

// migrate_accounts.c
int migrate_accounts_from_files(void);

// migrate_players.c
int migrate_players_from_files(void);
int sql_get_pid_by_name(const char *name);
int player_exists_in_db(const char *name);
int save_player_item(int pid, struct mig_obj *obj, int equip_slot, int container_id);

// migrate_lockers.c
int migrate_lockers_from_files(void);
int save_locker_item(int locker_id, struct mig_obj *obj, int container_id);

// migrate_ships.c
int migrate_ships_from_files(void);

// migrate_guilds.c
int migrate_guilds_from_files(void);

// migrate_misc.c
int migrate_corpses_from_files(void);
int migrate_saved_items_from_files(void);
int migrate_recipes_from_files(void);
int migrate_spellbooks_from_files(void);

#endif // MIGRATE_COMMON_H
