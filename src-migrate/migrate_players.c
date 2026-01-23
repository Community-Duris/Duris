// migrate_players.c
// player pfile migration for pfile migration tool

#include "migrate_common.h"

// thread-safe version of sql_get_pid_by_name
static int tsql_get_pid_by_name(const char *name) {
    if (!name) return 0;

    char *esc_name = tsql_escape_string(name);
    if (!esc_name) return 0;

    char query[256];
    snprintf(query, sizeof(query),
        "SELECT pid FROM player_data WHERE LOWER(name) = LOWER('%s') LIMIT 1", esc_name);
    free(esc_name);

    MYSQL_RES *result = tdb_query("%s", query);
    if (!result) return 0;

    MYSQL_ROW row = mysql_fetch_row(result);
    int pid = row ? atoi(row[0]) : 0;
    mysql_free_result(result);

    return pid;
}

int sql_get_pid_by_name(const char *name) {
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

int player_exists_in_db(const char *name) {
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

// wrapper for save_item_to_db for player items
int save_player_item(int pid, struct mig_obj *obj, int equip_slot, int container_id) {
    return save_item_to_db(obj, "player_items", "pid", pid, container_id, equip_slot);
}

// parse player status section
static int parse_player_status(char **bufptr, struct mig_player *p, int stat_vers) {
    char *str;
    int tmp, i;

    str = mig_getString(bufptr);
    if (str) { strncpy(p->name, str, 63); p->name[63] = 0; free(str); }

    p->pid = mig_getInt(bufptr);
    p->screen_length = MIG_GET_BYTE(*bufptr);

    str = mig_getString(bufptr);
    if (str) { strncpy(p->password, str, 63); p->password[63] = 0; free(str); }

    p->short_descr = mig_getString(bufptr);
    p->long_descr = mig_getString(bufptr);
    p->description = mig_getString(bufptr);
    p->title = mig_getString(bufptr);

    if (stat_vers < 33) {
        tmp = mig_getInt(bufptr);
        p->m_class = 1 << (tmp - 1);
        mig_getInt(bufptr);
    } else {
        p->m_class = (unsigned int)mig_getInt(bufptr);
    }

    if (stat_vers > 36) {
        p->secondary_class = (unsigned int)mig_getInt(bufptr);
        if (p->secondary_class == 0x80000000)
            p->secondary_class = 0;
    }
    if (stat_vers > 38)
        p->spec = MIG_GET_BYTE(*bufptr);

    p->race = MIG_GET_BYTE(*bufptr);
    p->racewar = MIG_GET_BYTE(*bufptr);
    p->level = MIG_GET_BYTE(*bufptr);
    if (stat_vers < 33) MIG_GET_BYTE(*bufptr);
    p->sex = MIG_GET_BYTE(*bufptr);
    p->weight = mig_getShort(bufptr);
    p->height = mig_getShort(bufptr);
    p->size = MIG_GET_BYTE(*bufptr);

    p->hometown = mig_getInt(bufptr);
    p->birthplace = mig_getInt(bufptr);
    p->orig_birthplace = mig_getInt(bufptr);

    p->birth_time = mig_getLong(bufptr);
    p->played_time = mig_getInt(bufptr);
    p->last_save = mig_getLong(bufptr);
    p->perm_aging = mig_getShort(bufptr);

    for (i = 0; i <= MIG_MAX_CIRCLE; i++)
        p->undead_slots[i] = MIG_GET_BYTE(*bufptr);

    mig_getInt(bufptr); // last_level, unused

    for (i = 0; i < MIG_NUMB_PC_TIMERS; i++)
        p->timers[i] = mig_getLong(bufptr);

    // trophy - skip if old format
    if (stat_vers < 45) {
        tmp = MIG_GET_BYTE(*bufptr);
        for (i = 0; i < tmp; i++) {
            mig_getInt(bufptr);
            mig_getInt(bufptr);
        }
    }

    // languages
    int num_langs = mig_getShort(bufptr);
    for (i = 0; i < num_langs && i < MIG_MAX_TONGUE; i++)
        p->languages[i] = MIG_GET_BYTE(*bufptr);
    for (; i < num_langs; i++)
        MIG_GET_BYTE(*bufptr);

    // intros
    int num_intros = mig_getShort(bufptr);
    for (i = 0; i < num_intros && i < MIG_MAX_INTRO; i++) {
        p->intro_pids[i] = mig_getInt(bufptr);
        p->intro_times[i] = mig_getLong(bufptr);
    }
    for (; i < num_intros; i++) {
        mig_getInt(bufptr);
        mig_getLong(bufptr);
    }

    // forged items
    if (stat_vers > 37 && stat_vers < 40) {
        for (i = 0; i < 100; i++)
            p->forged_items[i] = mig_getInt(bufptr);
    } else if (stat_vers > 39) {
        for (i = 0; i < MIG_MAX_FORGE_ITEMS; i++)
            p->forged_items[i] = mig_getInt(bufptr);
    }

    // base stats
    p->str = MIG_GET_BYTE(*bufptr);
    p->dex = MIG_GET_BYTE(*bufptr);
    p->agi = MIG_GET_BYTE(*bufptr);
    p->con = MIG_GET_BYTE(*bufptr);
    p->pow = MIG_GET_BYTE(*bufptr);
    p->intel = MIG_GET_BYTE(*bufptr);
    p->wis = MIG_GET_BYTE(*bufptr);
    p->cha = MIG_GET_BYTE(*bufptr);
    p->kar = MIG_GET_BYTE(*bufptr);
    p->luk = MIG_GET_BYTE(*bufptr);

    // points
    p->mana = mig_getShort(bufptr);
    p->base_mana = mig_getShort(bufptr);
    p->hp = mig_getShort(bufptr);
    p->spells_memmed = MIG_GET_BYTE(*bufptr);
    p->base_hp = mig_getShort(bufptr);
    p->vitality = mig_getShort(bufptr);
    p->base_vitality = mig_getShort(bufptr);

    // money
    p->copper = mig_getInt(bufptr);
    p->silver = mig_getInt(bufptr);
    p->gold = mig_getInt(bufptr);
    p->platinum = mig_getInt(bufptr);

    // experience
    p->exp = mig_getInt(bufptr);
    mig_getInt(bufptr); // max_exp unused
    p->epics = mig_getInt(bufptr);

    if (stat_vers >= 44)
        p->epic_skill_points = mig_getInt(bufptr);
    if (stat_vers > 46)
        p->skillpoints = mig_getInt(bufptr);
    if (stat_vers > 40)
        p->spell_bind_used = mig_getInt(bufptr);
    if (stat_vers < 43)
        mig_getInt(bufptr); // quaffed_level

    // flags
    p->act = mig_getInt(bufptr);
    p->act2 = mig_getInt(bufptr);
    if (stat_vers < 35) {
        mig_getInt(bufptr);
        mig_getInt(bufptr);
    }
    p->vote = mig_getInt(bufptr);
    p->alignment = mig_getInt(bufptr);
    mig_getInt(bufptr); // orig_align

    // guild
    p->prestige = mig_getShort(bufptr);
    p->assoc_id = mig_getShort(bufptr);
    p->guild_status = mig_getInt(bufptr);
    p->time_left_guild = mig_getLong(bufptr);
    p->nb_left_guild = MIG_GET_BYTE(*bufptr);

    if (stat_vers > 31)
        p->time_unspecced = mig_getLong(bufptr);

    if (stat_vers <= 35) {
        for (i = 0; i < 5; i++) {
            MIG_GET_BYTE(*bufptr);
            MIG_GET_BYTE(*bufptr);
        }
    }

    // frags
    if (stat_vers < 46) {
        mig_getLong(bufptr);
        mig_getLong(bufptr);
        p->frags = 0;
        p->oldfrags = 0;
    } else {
        p->frags = mig_getLong(bufptr);
        p->oldfrags = mig_getLong(bufptr);
        // sanitize corrupted frags (negative or overflow junk)
        if (p->frags < 0 || p->frags > 100000) p->frags = 0;
        if (p->oldfrags < 0 || p->oldfrags > 100000) p->oldfrags = 0;
    }

    if (stat_vers < 35) {
        mig_getShort(bufptr);
        mig_getShort(bufptr);
    }
    if (stat_vers < 34)
        mig_getInt(bufptr);
    if (stat_vers < 35)
        mig_getInt(bufptr);

    // granted commands
    p->num_granted_cmds = mig_getInt(bufptr);
    if (p->num_granted_cmds > 0) {
        p->granted_cmds = (int *)malloc(p->num_granted_cmds * sizeof(int));
        for (i = 0; i < p->num_granted_cmds; i++)
            p->granted_cmds[i] = mig_getInt(bufptr);
    }

    // conditions
    for (i = 0; i < MIG_MAX_COND; i++)
        p->conditions[i] = MIG_GET_BYTE(*bufptr);

    if (stat_vers < 35) {
        for (i = 0; i < 10; i++) // MAX_PETS
            mig_getInt(bufptr);
    }

    // poof messages
    p->poof_in = mig_getString(bufptr);
    p->poof_out = mig_getString(bufptr);
    if (stat_vers > 10) {
        p->poof_in_sound = mig_getString(bufptr);
        p->poof_out_sound = mig_getString(bufptr);
    }

    p->echo_toggle = MIG_GET_BYTE(*bufptr);
    p->prompt = mig_getShort(bufptr);
    p->wiz_invis = mig_getLong(bufptr);
    p->law_flags = mig_getLong(bufptr);
    p->wimpy = mig_getShort(bufptr);
    p->aggressive = mig_getShort(bufptr);
    p->highest_level = MIG_GET_BYTE(*bufptr);

    // bank
    p->bank_copper = mig_getInt(bufptr);
    p->bank_silver = mig_getInt(bufptr);
    p->bank_gold = mig_getInt(bufptr);
    p->bank_platinum = mig_getInt(bufptr);

    p->numb_deaths = mig_getLong(bufptr);

    // quest data
    if (stat_vers > 41) {
        p->quest_active = mig_getInt(bufptr);
        p->quest_mob_vnum = mig_getInt(bufptr);
        p->quest_type = mig_getInt(bufptr);
        p->quest_accomplished = mig_getInt(bufptr);
        p->quest_started = mig_getInt(bufptr);
        p->quest_zone_number = mig_getInt(bufptr);
        p->quest_giver = mig_getInt(bufptr);
        p->quest_level = mig_getInt(bufptr);
        p->quest_receiver = mig_getInt(bufptr);
        p->quest_shares_left = mig_getInt(bufptr);
        p->quest_kill_how_many = mig_getInt(bufptr);
        p->quest_kill_original = mig_getInt(bufptr);
        p->quest_map_room = mig_getInt(bufptr);
        p->quest_map_bought = mig_getInt(bufptr);
    }

    return 1;
}

// parse player skills section
static int parse_player_skills(char **bufptr, struct mig_player *p) {
    int skill_vers = MIG_GET_BYTE(*bufptr);
    if (skill_vers > SAV_SKILLVERS) return 0;

    int n = mig_getInt(bufptr);
    for (int i = 0; i < MIG_MAX_SKILLS; i++) {
        if (i < n) {
            p->skills_learned[i] = MIG_GET_BYTE(*bufptr);
            p->skills_taught[i] = MIG_GET_BYTE(*bufptr);
            MIG_GET_BYTE(*bufptr); // unused
        }
    }
    for (int i = MIG_MAX_SKILLS; i < n; i++) {
        MIG_GET_BYTE(*bufptr);
        MIG_GET_BYTE(*bufptr);
        MIG_GET_BYTE(*bufptr);
    }

    // skill usage loop
    int tmp;
    do { tmp = mig_getInt(bufptr); } while (tmp != 0);

    // skill usages
    n = mig_getShort(bufptr);
    for (int i = 0; i < n; i++) {
        mig_getLong(bufptr);
        MIG_GET_BYTE(*bufptr);
    }

    return 1;
}

// parse player witness section
static int parse_player_witness(char **bufptr) {
    int witness_vers = MIG_GET_BYTE(*bufptr);
    if (witness_vers > SAV_WTNSVERS) return 0;

    int count = mig_getInt(bufptr);
    for (int i = 0; i < count; i++) {
        mig_getInt(bufptr);
        mig_getInt(bufptr);
        char *s1 = mig_getString(bufptr); if (s1) free(s1);
        char *s2 = mig_getString(bufptr); if (s2) free(s2);
        mig_getLong(bufptr);
    }
    return 1;
}

// parse player affects section
static int parse_player_affects(char **bufptr, struct mig_player *p) {
    int aff_vers = MIG_GET_BYTE(*bufptr);
    if (aff_vers > SAV_AFFVERS) return 0;

    int count = mig_getShort(bufptr);
    struct mig_affect *last = NULL;

    for (int i = 0; i < count; i++) {
        struct mig_affect *af = (struct mig_affect *)malloc(sizeof(struct mig_affect));
        memset(af, 0, sizeof(struct mig_affect));

        if (aff_vers > 4) {
            if (aff_vers > 5) {
                unsigned char custom_msgs = MIG_GET_BYTE(*bufptr);
                if (custom_msgs & 1)
                    af->wear_off_char = mig_getString(bufptr);
                if (custom_msgs & 2)
                    af->wear_off_room = mig_getString(bufptr);
                af->type = mig_getShort(bufptr);
            } else {
                af->type = mig_getInt(bufptr);
            }
            af->duration = mig_getInt(bufptr);
            af->flags = mig_getShort(bufptr);
            af->modifier = mig_getInt(bufptr);
            af->location = MIG_GET_BYTE(*bufptr);
            af->bitvector1 = mig_getLong(bufptr);
            af->bitvector2 = mig_getLong(bufptr);
            af->bitvector3 = mig_getLong(bufptr);
            af->bitvector4 = mig_getLong(bufptr);
            af->bitvector5 = mig_getLong(bufptr);
            mig_getLong(bufptr); // bitvector6 unused

            if (aff_vers > 7)
                af->level = mig_getShort(bufptr);
            else
                af->level = p->level;
        } else {
            af->type = mig_getInt(bufptr);
            af->duration = mig_getShort(bufptr);
            af->modifier = mig_getInt(bufptr);
            af->location = MIG_GET_BYTE(*bufptr);
            mig_getInt(bufptr); // loc2
            af->bitvector1 = mig_getLong(bufptr);
            af->bitvector2 = mig_getLong(bufptr);
            af->bitvector3 = mig_getLong(bufptr);
            af->bitvector4 = mig_getLong(bufptr);
            af->bitvector5 = mig_getLong(bufptr);
            af->flags = 0;
            if (aff_vers == 4) {
                mig_getLong(bufptr); // bitvector6
                long short_dur = mig_getLong(bufptr);
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

// helper: format nullable string for sql
static void fmt_sql_str(char *out, size_t sz, const char *esc) {
    if (esc)
        snprintf(out, sz, "'%s'", esc);
    else
        strcpy(out, "NULL");
}

// thread-local flag for using thread-safe db functions
static __thread int use_thread_db = 0;

// wrapper functions that select global vs thread-local based on flag
static char *my_escape(const char *str) {
    return use_thread_db ? tsql_escape_string(str) : sql_escape_string(str);
}

static bool my_qry(const char *format, ...) {
    char query[65536];
    va_list args;
    va_start(args, format);
    vsnprintf(query, sizeof(query), format, args);
    va_end(args);
    return use_thread_db ? tqry("%s", query) : qry("%s", query);
}

static MYSQL_RES *my_db_query(const char *format, ...) {
    char query[65536];
    va_list args;
    va_start(args, format);
    vsnprintf(query, sizeof(query), format, args);
    va_end(args);
    return use_thread_db ? tdb_query("%s", query) : db_query("%s", query);
}

static int my_get_pid_by_name(const char *name) {
    return use_thread_db ? tsql_get_pid_by_name(name) : sql_get_pid_by_name(name);
}

// save complete player to database - uses UPDATE for existing, INSERT for new
static int save_player_to_db(struct mig_player *p) {
    if (!p || !p->name[0]) return 0;

    char *esc_name = my_escape(p->name);
    if (!esc_name) return 0;

    // check if player already exists FIRST
    int pid = my_get_pid_by_name(p->name);

    char *esc_short = p->short_descr ? my_escape(p->short_descr) : NULL;
    char *esc_long = p->long_descr ? my_escape(p->long_descr) : NULL;
    char *esc_desc = p->description ? my_escape(p->description) : NULL;
    char *esc_title = p->title ? my_escape(p->title) : NULL;
    char *esc_poof_in = p->poof_in ? my_escape(p->poof_in) : NULL;
    char *esc_poof_out = p->poof_out ? my_escape(p->poof_out) : NULL;
    char *esc_poof_in_snd = p->poof_in_sound ? my_escape(p->poof_in_sound) : NULL;
    char *esc_poof_out_snd = p->poof_out_sound ? my_escape(p->poof_out_sound) : NULL;

    char short_str[2048], long_str[2048], desc_str[4096], title_str[1024];
    char poof_in_str[1024], poof_out_str[1024], poof_in_snd_str[1024], poof_out_snd_str[1024];
    fmt_sql_str(short_str, sizeof(short_str), esc_short);
    fmt_sql_str(long_str, sizeof(long_str), esc_long);
    fmt_sql_str(desc_str, sizeof(desc_str), esc_desc);
    fmt_sql_str(title_str, sizeof(title_str), esc_title);
    fmt_sql_str(poof_in_str, sizeof(poof_in_str), esc_poof_in);
    fmt_sql_str(poof_out_str, sizeof(poof_out_str), esc_poof_out);
    fmt_sql_str(poof_in_snd_str, sizeof(poof_in_snd_str), esc_poof_in_snd);
    fmt_sql_str(poof_out_snd_str, sizeof(poof_out_snd_str), esc_poof_out_snd);

    char query[16384];

    if (pid > 0) {
        // player exists - use UPDATE instead of DELETE+INSERT
        snprintf(query, sizeof(query),
            "UPDATE player_data SET "
            "short_descr=%s, long_descr=%s, description=%s, title=%s, "
            "m_class=%u, secondary_class=%u, spec=%d, race=%d, racewar=%d, level=%d, sex=%d, "
            "weight=%d, height=%d, size=%d, hometown=%d, birthplace=%d, orig_birthplace=%d, last_room=%d, "
            "birth_time=%ld, played_time=%ld, last_save=%ld, perm_aging=%d, "
            "base_str=%d, base_dex=%d, base_agi=%d, base_con=%d, base_pow=%d, base_int=%d, base_wis=%d, base_cha=%d, base_kar=%d, base_luk=%d, "
            "mana=%d, base_mana=%d, hit_diff=%d, base_hit=%d, vitality=%d, base_vitality=%d, spells_memmed_extra=%d, "
            "copper=%d, silver=%d, gold=%d, platinum=%d, bank_copper=%d, bank_silver=%d, bank_gold=%d, bank_platinum=%d, "
            "exp=%d, epics=%d, epic_skill_points=%d, skillpoints=%d, spell_bind_used=%d, "
            "act=%u, act2=%u, act3=%u, vote=%d, alignment=%d, prestige=%d, assoc_id=%d, guild_status=%d, "
            "time_left_guild=%ld, nb_left_guild=%d, time_unspecced=%ld, frags=%ld, oldfrags=%ld, numb_deaths=%ld, "
            "condition_0=%d, condition_1=%d, condition_2=%d, condition_3=%d, condition_4=%d, "
            "poof_in=%s, poof_out=%s, poof_in_sound=%s, poof_out_sound=%s, "
            "echo_toggle=%d, prompt=%d, wiz_invis=%ld, law_flags=%lu, wimpy=%d, aggressive=%d, highest_level=%d, screen_length=%d, "
            "quest_active=%d, quest_mob_vnum=%d, quest_type=%d, quest_accomplished=%d, quest_started=%d, "
            "quest_zone_number=%d, quest_giver=%d, quest_level=%d, quest_receiver=%d, "
            "quest_shares_left=%d, quest_kill_how_many=%d, quest_kill_original=%d, quest_map_room=%d, quest_map_bought=%d "
            "WHERE pid=%d",
            short_str, long_str, desc_str, title_str,
            p->m_class, p->secondary_class, p->spec, p->race, p->racewar, p->level, p->sex,
            p->weight, p->height, p->size, p->hometown, p->birthplace, p->orig_birthplace, p->last_room,
            p->birth_time, p->played_time, p->last_save, p->perm_aging,
            p->str, p->dex, p->agi, p->con, p->pow, p->intel, p->wis, p->cha, p->kar, p->luk,
            p->mana, p->base_mana, p->hp - p->base_hp, p->base_hp, p->vitality, p->base_vitality, p->spells_memmed,
            p->copper, p->silver, p->gold, p->platinum,
            p->bank_copper, p->bank_silver, p->bank_gold, p->bank_platinum,
            p->exp, p->epics, p->epic_skill_points, p->skillpoints, p->spell_bind_used,
            p->act, p->act2, p->act3, p->vote, p->alignment, p->prestige, p->assoc_id, p->guild_status,
            p->time_left_guild, p->nb_left_guild, p->time_unspecced, p->frags, p->oldfrags, p->numb_deaths,
            p->conditions[0], p->conditions[1], p->conditions[2], p->conditions[3], p->conditions[4],
            poof_in_str, poof_out_str, poof_in_snd_str, poof_out_snd_str,
            p->echo_toggle, p->prompt, p->wiz_invis, p->law_flags, p->wimpy, (short)p->aggressive, p->highest_level, p->screen_length,
            p->quest_active, p->quest_mob_vnum, p->quest_type, p->quest_accomplished, p->quest_started,
            p->quest_zone_number, p->quest_giver, p->quest_level, p->quest_receiver,
            p->quest_shares_left, p->quest_kill_how_many, p->quest_kill_original, p->quest_map_room, p->quest_map_bought,
            pid);

        // for existing player, delete items (they'll be re-inserted fresh)
        my_qry("DELETE FROM player_items WHERE pid = %d", pid);
        my_qry("DELETE FROM player_affects WHERE pid = %d", pid);
    } else {
        // new player - use INSERT
        snprintf(query, sizeof(query),
            "INSERT INTO player_data ("
            "name, short_descr, long_descr, description, title, "
            "m_class, secondary_class, spec, race, racewar, level, sex, "
            "weight, height, size, hometown, birthplace, orig_birthplace, last_room,"
            "birth_time, played_time, last_save, perm_aging, "
            "base_str, base_dex, base_agi, base_con, base_pow, base_int, base_wis, base_cha, base_kar, base_luk, "
            "mana, base_mana, hit_diff, base_hit, vitality, base_vitality, spells_memmed_extra, "
            "copper, silver, gold, platinum, bank_copper, bank_silver, bank_gold, bank_platinum, "
            "exp, epics, epic_skill_points, skillpoints, spell_bind_used, "
            "act, act2, act3, vote, alignment, prestige, assoc_id, guild_status,"
            "time_left_guild, nb_left_guild, time_unspecced, frags, oldfrags, numb_deaths, "
            "condition_0, condition_1, condition_2, condition_3, condition_4, "
            "poof_in, poof_out, poof_in_sound, poof_out_sound, "
            "echo_toggle, prompt, wiz_invis, law_flags, wimpy, aggressive, highest_level, screen_length, "
            "quest_active, quest_mob_vnum, quest_type, quest_accomplished, quest_started, "
            "quest_zone_number, quest_giver, quest_level, quest_receiver, "
            "quest_shares_left, quest_kill_how_many, quest_kill_original, quest_map_room, quest_map_bought"
            ") VALUES ("
            "'%s', %s, %s, %s, %s, "
            "%u, %u, %d, %d, %d, %d, %d, "
            "%d, %d, %d, %d, %d, %d, %d, "
            "%ld, %ld, %ld, %d, "
            "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
            "%d, %d, %d, %d, %d, %d, %d, "
            "%d, %d, %d, %d, %d, %d, %d, %d, "
            "%d, %d, %d, %d, %d, "
            "%u, %u, %u, %d, %d, %d, %d, %d, "
            "%ld, %d, %ld, %ld, %ld, %ld, "
            "%d, %d, %d, %d, %d, "
            "%s, %s, %s, %s, "
            "%d, %d, %ld, %lu, %d, %d, %d, %d, "
            "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d"
            ")",
            esc_name, short_str, long_str, desc_str, title_str,
            p->m_class, p->secondary_class, p->spec, p->race, p->racewar, p->level, p->sex,
            p->weight, p->height, p->size, p->hometown, p->birthplace, p->orig_birthplace, p->last_room,
            p->birth_time, p->played_time, p->last_save, p->perm_aging,
            p->str, p->dex, p->agi, p->con, p->pow, p->intel, p->wis, p->cha, p->kar, p->luk,
            p->mana, p->base_mana, p->hp - p->base_hp, p->base_hp, p->vitality, p->base_vitality, p->spells_memmed,
            p->copper, p->silver, p->gold, p->platinum,
            p->bank_copper, p->bank_silver, p->bank_gold, p->bank_platinum,
            p->exp, p->epics, p->epic_skill_points, p->skillpoints, p->spell_bind_used,
            p->act, p->act2, p->act3, p->vote, p->alignment, p->prestige, p->assoc_id, p->guild_status,
            p->time_left_guild, p->nb_left_guild, p->time_unspecced, p->frags, p->oldfrags, p->numb_deaths,
            p->conditions[0], p->conditions[1], p->conditions[2], p->conditions[3], p->conditions[4],
            poof_in_str, poof_out_str, poof_in_snd_str, poof_out_snd_str,
            p->echo_toggle, p->prompt, p->wiz_invis, p->law_flags, p->wimpy, (short)p->aggressive, p->highest_level, p->screen_length,
            p->quest_active, p->quest_mob_vnum, p->quest_type, p->quest_accomplished, p->quest_started,
            p->quest_zone_number, p->quest_giver, p->quest_level, p->quest_receiver,
            p->quest_shares_left, p->quest_kill_how_many, p->quest_kill_original, p->quest_map_room, p->quest_map_bought);
    }

    // free escaped strings
    free(esc_name);
    if (esc_short) free(esc_short);
    if (esc_long) free(esc_long);
    if (esc_desc) free(esc_desc);
    if (esc_title) free(esc_title);
    if (esc_poof_in) free(esc_poof_in);
    if (esc_poof_out) free(esc_poof_out);
    if (esc_poof_in_snd) free(esc_poof_in_snd);
    if (esc_poof_out_snd) free(esc_poof_out_snd);

    if (!my_qry("%s", query))
        return 0;

    // for new players, get the pid
    if (pid <= 0) {
        pid = sql_get_pid_by_name(p->name);
        if (pid <= 0) return 0;
    }

    // === BATCHED ARRAY INSERTS ===
    // instead of 263 individual queries, batch into ~8 multi-value inserts

    // batch skills
    char values[65536];
    int len = 0;
    for (int i = 0; i < MIG_MAX_SKILLS; i++) {
        if (p->skills_learned[i] > 0 || p->skills_taught[i] > 0) {
            len += snprintf(values + len, sizeof(values) - len, "(%d,%d,%d,%d),",
                pid, i, p->skills_learned[i], p->skills_taught[i]);
        }
    }
    if (len > 0) {
        values[len - 1] = '\0';
        my_qry("INSERT INTO player_skills (pid,skill_id,learned,taught) VALUES %s "
            "ON DUPLICATE KEY UPDATE learned=VALUES(learned),taught=VALUES(taught)", values);
    }

    // batch languages
    len = 0;
    for (int i = 0; i < MIG_MAX_TONGUE; i++) {
        if (p->languages[i] > 0) {
            len += snprintf(values + len, sizeof(values) - len, "(%d,%d,%d),",
                pid, i, p->languages[i]);
        }
    }
    if (len > 0) {
        values[len - 1] = '\0';
        my_qry("INSERT INTO player_languages (pid,tongue_id,proficiency) VALUES %s "
            "ON DUPLICATE KEY UPDATE proficiency=VALUES(proficiency)", values);
    }

    // batch timers
    len = 0;
    for (int i = 0; i < MIG_NUMB_PC_TIMERS; i++) {
        if (p->timers[i] != 0) {
            len += snprintf(values + len, sizeof(values) - len, "(%d,%d,%ld),",
                pid, i, p->timers[i]);
        }
    }
    if (len > 0) {
        values[len - 1] = '\0';
        my_qry("INSERT INTO player_timers (pid,timer_id,timer_value) VALUES %s "
            "ON DUPLICATE KEY UPDATE timer_value=VALUES(timer_value)", values);
    }

    // batch undead slots
    len = 0;
    for (int i = 0; i <= MIG_MAX_CIRCLE; i++) {
        if (p->undead_slots[i] > 0) {
            len += snprintf(values + len, sizeof(values) - len, "(%d,%d,%d),",
                pid, i, p->undead_slots[i]);
        }
    }
    if (len > 0) {
        values[len - 1] = '\0';
        my_qry("INSERT INTO player_undead_slots (pid,circle,slots) VALUES %s "
            "ON DUPLICATE KEY UPDATE slots=VALUES(slots)", values);
    }

    // batch forged items
    len = 0;
    for (int i = 0; i < MIG_MAX_FORGE_ITEMS; i++) {
        if (p->forged_items[i] != 0) {
            len += snprintf(values + len, sizeof(values) - len, "(%d,%d,%d),",
                pid, i, p->forged_items[i]);
        }
    }
    if (len > 0) {
        values[len - 1] = '\0';
        my_qry("INSERT INTO player_forged_items (pid,forge_index,item_vnum) VALUES %s "
            "ON DUPLICATE KEY UPDATE item_vnum=VALUES(item_vnum)", values);
    }

    // batch intros
    len = 0;
    for (int i = 0; i < MIG_MAX_INTRO; i++) {
        if (p->intro_pids[i] != 0) {
            len += snprintf(values + len, sizeof(values) - len, "(%d,%d,%d,0),",
                pid, i, p->intro_pids[i]);
        }
    }
    if (len > 0) {
        values[len - 1] = '\0';
        my_qry("INSERT INTO player_intros (pid,intro_index,intro_pid,intro_time) VALUES %s "
            "ON DUPLICATE KEY UPDATE intro_pid=VALUES(intro_pid)", values);
    }

    // batch granted commands
    if (p->num_granted_cmds > 0) {
        len = 0;
        for (int i = 0; i < p->num_granted_cmds; i++) {
            len += snprintf(values + len, sizeof(values) - len, "(%d,%d),",
                pid, p->granted_cmds[i]);
        }
        if (len > 0) {
            values[len - 1] = '\0';
            my_qry("INSERT INTO player_granted_cmds (pid,cmd_num) VALUES %s "
                "ON DUPLICATE KEY UPDATE cmd_num=cmd_num", values);
        }
    }

    // affects - these have string fields so batch them too
    len = 0;
    for (struct mig_affect *af = p->affects; af; af = af->next) {
        char *esc_woc = af->wear_off_char ? sql_escape_string(af->wear_off_char) : NULL;
        char *esc_wor = af->wear_off_room ? sql_escape_string(af->wear_off_room) : NULL;
        char woc_str[256], wor_str[256];
        if (esc_woc) { snprintf(woc_str, sizeof(woc_str), "'%s'", esc_woc); free(esc_woc); }
        else strcpy(woc_str, "NULL");
        if (esc_wor) { snprintf(wor_str, sizeof(wor_str), "'%s'", esc_wor); free(esc_wor); }
        else strcpy(wor_str, "NULL");

        len += snprintf(values + len, sizeof(values) - len,
            "(%d,%d,%d,%d,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%s,%s),",
            pid, af->type, af->duration, af->flags, af->modifier, af->location, af->level,
            af->bitvector1, af->bitvector2, af->bitvector3, af->bitvector4, af->bitvector5,
            woc_str, wor_str);
    }
    if (len > 0) {
        values[len - 1] = '\0';
        my_qry("INSERT INTO player_affects (pid,type,duration,flags,modifier,location,level,"
            "bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,custom_msg_char,custom_msg_room) VALUES %s",
            values);
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

    char *bufptr = buffer;

    // check pfile header
    int save_vers = MIG_GET_BYTE(bufptr);
    if (save_vers != SAV_SAVEVERS) {
        printf("  wrong save version: %d\n", save_vers);
        return NULL;
    }

    int ss = MIG_GET_BYTE(bufptr);
    int is = MIG_GET_BYTE(bufptr);
    int ls = MIG_GET_BYTE(bufptr);
    if (ss != 2 || is != 4 || ls != 8) {
        printf("  wrong machine format: %d/%d/%d\n", ss, is, ls);
        return NULL;
    }

    MIG_GET_BYTE(bufptr); // rent type

    // read offsets
    int skill_off = mig_getInt(&bufptr);
    int witness_off = mig_getInt(&bufptr);
    int affect_off = mig_getInt(&bufptr);
    int item_off = mig_getInt(&bufptr);
    int size_off = mig_getInt(&bufptr);
    unsigned int tmp_act3 = (unsigned int)mig_getInt(&bufptr); // act3/surname
    int tmp_last_room = mig_getInt(&bufptr); // room player was saved in
    mig_getLong(&bufptr); // save time

    // validate offsets
    if (skill_off >= size || witness_off >= size || affect_off >= size ||
        item_off >= size || size_off > size) {
        printf("  invalid offsets: skill=%d witness=%d affect=%d item=%d size=%d (filesize=%d)\n",
               skill_off, witness_off, affect_off, item_off, size_off, size);
        return NULL;
    }

    // allocate player
    struct mig_player *p = (struct mig_player *)malloc(sizeof(struct mig_player));
    memset(p, 0, sizeof(struct mig_player));
    p->act3 = tmp_act3;
    p->last_room = tmp_last_room;

    // parse status
    int stat_vers = MIG_GET_BYTE(bufptr);
    if (stat_vers > SAV_STATVERS) {
        printf("  wrong stat version: %d\n", stat_vers);
        free(p);
        return NULL;
    }

    if (!parse_player_status(&bufptr, p, stat_vers)) {
        free_mig_player(p);
        return NULL;
    }

    // parse skills
    bufptr = buffer + skill_off;
    if (!parse_player_skills(&bufptr, p)) {
        free_mig_player(p);
        return NULL;
    }

    // parse witness (skip)
    bufptr = buffer + witness_off;
    parse_player_witness(&bufptr);

    // parse affects
    bufptr = buffer + affect_off;
    if (!parse_player_affects(&bufptr, p)) {
        free_mig_player(p);
        return NULL;
    }

    // parse items
    bufptr = buffer + item_off;
    if (item_off >= size || item_off < 0) {
        printf("  invalid item_off: %d (size: %d)\n", item_off, size);
        free_mig_player(p);
        return NULL;
    }
    if (!parse_player_items(&bufptr, p)) {
        free_mig_player(p);
        return NULL;
    }

    return p;
}

// count player files for progress bar
static int count_player_files(void) {
    int total = 0;
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
            total++;
        }
        closedir(dir);
    }
    return total;
}

// collect all player files into array
static char **collect_player_files(int *count) {
    int total = count_player_files();
    char **files = (char **)malloc(sizeof(char *) * total);
    int idx = 0;

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

            files[idx++] = strdup(filepath);
        }
        closedir(dir);
    }
    *count = idx;
    return files;
}

// parallel worker thread
static void *player_worker(void *arg) {
    struct work_queue *wq = (struct work_queue *)arg;

    // enable thread-local db for this worker
    use_thread_db = 1;
    item_use_thread_db = 1;
    init_thread_db();

    // check if connection succeeded
    if (!get_thread_db()) {
        fprintf(stderr, "worker thread: failed to get db connection, exiting\n");
        return NULL;
    }

    // set bulk import mode on this thread's connection
    tqry("SET autocommit=0");
    tqry("SET unique_checks=0");
    tqry("SET foreign_key_checks=0");

    char *filepath;
    int processed = 0;
    while ((filepath = work_queue_get(wq)) != NULL) {
        struct mig_player *p = parse_player_pfile(filepath);
        int success = 0;
        if (p) {
            int pid = save_player_to_db(p);
            success = (pid > 0);
            free_mig_player(p);
        }
        work_queue_done(wq, success);

        // commit every 20 players per thread to avoid redo log overflow
        if (++processed % 20 == 0) {
            tqry("COMMIT");
        }
    }

    tqry("COMMIT");  // final commit for this thread
    close_thread_db();
    return NULL;
}

// migrate all player pfiles
int migrate_players_from_files(void) {
    int file_count = 0;
    char **files = collect_player_files(&file_count);

    struct progress_bar pb;
    progress_init(&pb, file_count, "players");

    // use parallel workers if requested
    if (g_num_threads > 1) {
        struct work_queue wq;
        work_queue_init(&wq, files, file_count, &pb);

        pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * g_num_threads);
        for (int i = 0; i < g_num_threads; i++) {
            pthread_create(&threads[i], NULL, player_worker, &wq);
        }

        for (int i = 0; i < g_num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        free(threads);

        progress_finish(&pb);
        printf("players: %d migrated, %d errors\n", wq.success, wq.errors);

        // cleanup
        work_queue_destroy(&wq);
        for (int i = 0; i < file_count; i++) free(files[i]);
        free(files);

        return wq.success;
    }

    // single-threaded fallback
    int count = 0, errors = 0;
    for (int i = 0; i < file_count; i++) {
        struct mig_player *p = parse_player_pfile(files[i]);
        if (!p) {
            errors++;
        } else {
            int pid = save_player_to_db(p);
            if (pid > 0) count++;
            else errors++;
            free_mig_player(p);
        }
        progress_update(&pb, i + 1);

        // commit every 50 players to flush redo log
        if ((i + 1) % 50 == 0) {
            qry("COMMIT");
        }
    }
    qry("COMMIT"); // final commit

    progress_finish(&pb);
    printf("players: %d migrated, %d errors\n", count, errors);

    for (int i = 0; i < file_count; i++) free(files[i]);
    free(files);

    return count;
}
