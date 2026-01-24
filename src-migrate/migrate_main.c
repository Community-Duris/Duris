// migrate_main.c
// main entry point for pfile migration tool

#include "migrate_common.h"

static void print_usage(const char *prog) {
    printf("usage: %s [options]\n", prog);
    printf("options:\n");
    printf("  --all        run all migrations (default)\n");
    printf("  --accounts   migrate accounts only\n");
    printf("  --players    migrate players only\n");
    printf("  --lockers    migrate lockers only\n");
    printf("  --ships      migrate ships only\n");
    printf("  --guilds     migrate guilds only\n");
    printf("  --recipes    migrate recipes only\n");
    printf("  --spellbooks migrate spellbooks only\n");
    printf("  --shopkeepers migrate shopkeepers only\n");
    printf("  --frag       populate frag_leaderboard only\n");
    printf("  --clean      clear relevant tables before migration\n");
    printf("  -j N         use N parallel threads (default: 1)\n");
    printf("  --help       show this help\n");
}

// clean functions for each category
static void clean_players(void) {
    printf("  clearing player tables...\n");
    qry("DELETE FROM player_items");
    qry("DELETE FROM player_affects");
    qry("DELETE FROM player_skills");
    qry("DELETE FROM player_languages");
    qry("DELETE FROM player_timers");
    qry("DELETE FROM player_undead_slots");
    qry("DELETE FROM player_forged_items");
    qry("DELETE FROM player_intros");
    qry("DELETE FROM player_granted_cmds");
    qry("DELETE FROM account_characters");
    qry("DELETE FROM player_data");
}

static void clean_accounts(void) {
    printf("  clearing account tables...\n");
    // account_characters is cleared by clean_players (owns the pid reference)
    qry("DELETE FROM account_ips");
    qry("DELETE FROM accounts");
}

static void clean_lockers(void) {
    printf("  clearing locker tables...\n");
    qry("DELETE FROM locker_items");
    qry("DELETE FROM lockers");
}

static void clean_ships(void) {
    printf("  clearing ship tables...\n");
    qry("DELETE FROM ship_slots");
    qry("DELETE FROM ships");
}

static void clean_guilds(void) {
    printf("  clearing guild tables...\n");
    qry("DELETE FROM guild_members");
    qry("DELETE FROM guilds");
}

static void clean_recipes(void) {
    printf("  clearing recipe tables...\n");
    qry("DELETE FROM player_recipes");
}

static void clean_spellbooks(void) {
    printf("  clearing spellbook tables...\n");
    qry("DELETE FROM player_spellbooks");
}

static void clean_shopkeepers(void) {
    printf("  clearing shopkeeper tables...\n");
    qry("DELETE FROM shopkeeper_items");
    qry("DELETE FROM shopkeeper_affects");
    qry("DELETE FROM shopkeepers");
}

static void clean_frag_leaderboard(void) {
    printf("  clearing frag_leaderboard...\n");
    qry("DELETE FROM frag_leaderboard");
}

static void clean_corpses(void) {
    printf("  clearing corpse tables...\n");
    qry("DELETE FROM corpse_items");
    qry("DELETE FROM corpses");
}

static void clean_saved_items(void) {
    printf("  clearing saved_items table...\n");
    qry("DELETE FROM saved_items");
}

// save old pid->name mapping before player migration
// use players_core because prod artifacts reference players_core.pid
static void save_old_pid_mapping(void) {
    qry("DROP TABLE IF EXISTS _old_pid_map");
    qry("CREATE TEMPORARY TABLE _old_pid_map AS SELECT pid, name FROM players_core WHERE active = 1");
}

// remap artifact locations after player migration
static void remap_artifact_locations(void) {
    printf("remapping artifact locations to new pids...\n");

    // update artifacts table
    int updated = 0;
    MYSQL_RES *res = db_query(
        "SELECT a.vnum, old.name, new.pid "
        "FROM artifacts a "
        "JOIN _old_pid_map old ON a.location = old.pid "
        "JOIN player_data new ON LOWER(old.name) = LOWER(new.name) AND new.active = 1 "
        "WHERE (a.locType = 3 OR a.locType = 5) AND a.owned = 'Y'"  // 3=OnPC, 5=OnCorpse
    );
    if (res) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            qry("UPDATE artifacts SET location = %s WHERE vnum = %s", row[2], row[0]);
            updated++;
        }
        mysql_free_result(res);
    }
    printf("  artifacts: %d remapped\n", updated);

    // update artifacts_mortal table
    updated = 0;
    res = db_query(
        "SELECT a.vnum, old.name, new.pid "
        "FROM artifacts_mortal a "
        "JOIN _old_pid_map old ON a.location = old.pid "
        "JOIN player_data new ON LOWER(old.name) = LOWER(new.name) AND new.active = 1 "
        "WHERE (a.locType = 3 OR a.locType = 5) AND a.owned = 'Y'"  // 3=OnPC, 5=OnCorpse
    );
    if (res) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            qry("UPDATE artifacts_mortal SET location = %s WHERE vnum = %s", row[2], row[0]);
            updated++;
        }
        mysql_free_result(res);
    }
    printf("  artifacts_mortal: %d remapped\n", updated);

    // update artifact_bind table
    updated = 0;
    res = db_query(
        "SELECT ab.vnum, old.name, new.pid "
        "FROM artifact_bind ab "
        "JOIN _old_pid_map old ON ab.owner_pid = old.pid "
        "JOIN player_data new ON LOWER(old.name) = LOWER(new.name) AND new.active = 1 "
        "WHERE ab.owner_pid > 0"
    );
    if (res) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            qry("UPDATE artifact_bind SET owner_pid = %s WHERE vnum = %s", row[2], row[0]);
            updated++;
        }
        mysql_free_result(res);
    }
    printf("  artifact_bind: %d remapped\n", updated);

    // fallback: sync artifacts.location from artifact_bind for on-pc artifacts
    // this handles cases where the old pid mapping failed
    updated = 0;
    res = db_query(
        "SELECT a.vnum, ab.owner_pid "
        "FROM artifacts a "
        "JOIN artifact_bind ab ON a.vnum = ab.vnum "
        "WHERE a.locType = 3 AND ab.owner_pid > 0 AND a.location != ab.owner_pid"
    );
    if (res) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            qry("UPDATE artifacts SET location = %s WHERE vnum = %s", row[1], row[0]);
            updated++;
        }
        mysql_free_result(res);
    }
    if (updated > 0)
        printf("  artifacts synced from artifact_bind: %d\n", updated);

    // same for artifacts_mortal
    updated = 0;
    res = db_query(
        "SELECT a.vnum, ab.owner_pid "
        "FROM artifacts_mortal a "
        "JOIN artifact_bind ab ON a.vnum = ab.vnum "
        "WHERE a.locType = 3 AND ab.owner_pid > 0 AND a.location != ab.owner_pid"
    );
    if (res) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            qry("UPDATE artifacts_mortal SET location = %s WHERE vnum = %s", row[1], row[0]);
            updated++;
        }
        mysql_free_result(res);
    }
    if (updated > 0)
        printf("  artifacts_mortal synced from artifact_bind: %d\n", updated);
}

// remap ip_info pids after player migration
static void remap_ip_info(void) {
    printf("remapping ip_info to new pids...\n");

    // delete ip_info entries that would block remapping (their pid = someone else's new_pid)
    qry("DELETE ii_blocker FROM ip_info ii_blocker "
        "JOIN ( "
        "  SELECT new.pid as new_pid "
        "  FROM ip_info ii "
        "  JOIN _old_pid_map old ON ii.pid = old.pid "
        "  JOIN player_data new ON LOWER(old.name) = LOWER(new.name) AND new.active = 1 "
        ") remap ON ii_blocker.pid = remap.new_pid");

    int updated = 0;
    MYSQL_RES *res = db_query(
        "SELECT ii.pid, old.name, new.pid as new_pid "
        "FROM ip_info ii "
        "JOIN _old_pid_map old ON ii.pid = old.pid "
        "JOIN player_data new ON LOWER(old.name) = LOWER(new.name) AND new.active = 1"
    );
    if (res) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            qry("UPDATE ip_info SET pid = %s WHERE pid = %s", row[2], row[0]);
            updated++;
        }
        mysql_free_result(res);
    }
    printf("  ip_info: %d remapped\n", updated);
}

// update player_data.last_ip from ip_info
static void update_player_last_ip(void) {
    printf("updating player_data.last_ip from ip_info...\n");

    qry("UPDATE player_data pd "
        "JOIN ip_info ii ON pd.pid = ii.pid "
        "SET pd.last_ip = INET_ATON(ii.last_ip) "
        "WHERE pd.last_ip = 0 AND ii.last_ip IS NOT NULL AND ii.last_ip != ''");

    MYSQL_RES *res = db_query("SELECT ROW_COUNT()");
    if (res) {
        MYSQL_ROW row = mysql_fetch_row(res);
        printf("  player_data.last_ip: %s updated\n", row ? row[0] : "0");
        mysql_free_result(res);
    }
}

// cleanup old pid mapping temp table
static void cleanup_old_pid_mapping(void) {
    qry("DROP TABLE IF EXISTS _old_pid_map");
}

// populate frag leaderboard from player data
static int populate_frag_leaderboard(void) {
    printf("populating frag_leaderboard from player_data...\n");

    qry("DELETE FROM frag_leaderboard");

    // filter out corrupted frags (> 100000 = unsigned overflow junk)
    // m_class is bitmask so use log2 to get class id
    int ok = qry(
        "INSERT INTO frag_leaderboard (pid, account_name, char_name, total_frags, racewar, race, class, level) "
        "SELECT pd.pid, COALESCE(pd.account_name, ''), pd.name, pd.frags, pd.racewar, "
        "       COALESCE(r.name, 'Unknown'), COALESCE(c.name, 'Unknown'), pd.level "
        "FROM player_data pd "
        "LEFT JOIN races r ON pd.race = r.id "
        "LEFT JOIN classes c ON FLOOR(LOG2(pd.m_class)) + 1 = c.id "
        "WHERE pd.frags > 0 AND pd.frags < 100000"
    );

    if (!ok) {
        printf("  failed to populate frag_leaderboard\n");
        return 0;
    }

    MYSQL_RES *result = db_query("SELECT COUNT(*) FROM frag_leaderboard");
    int count = 0;
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row) count = atoi(row[0]);
        mysql_free_result(result);
    }

    printf("frag_leaderboard: %d entries populated\n", count);
    return count;
}

int main(int argc, char **argv) {
    // parse args
    int do_all = (argc == 1);
    int do_accounts = 0, do_players = 0, do_lockers = 0, do_ships = 0;
    int do_guilds = 0, do_recipes = 0, do_spellbooks = 0, do_shopkeepers = 0, do_frag = 0;
    int do_clean = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0) do_all = 1;
        else if (strcmp(argv[i], "--accounts") == 0) do_accounts = 1;
        else if (strcmp(argv[i], "--players") == 0) do_players = 1;
        else if (strcmp(argv[i], "--lockers") == 0) do_lockers = 1;
        else if (strcmp(argv[i], "--ships") == 0) do_ships = 1;
        else if (strcmp(argv[i], "--guilds") == 0) do_guilds = 1;
        else if (strcmp(argv[i], "--recipes") == 0) do_recipes = 1;
        else if (strcmp(argv[i], "--spellbooks") == 0) do_spellbooks = 1;
        else if (strcmp(argv[i], "--shopkeepers") == 0) do_shopkeepers = 1;
        else if (strcmp(argv[i], "--frag") == 0) do_frag = 1;
        else if (strcmp(argv[i], "--clean") == 0) do_clean = 1;
        else if (strcmp(argv[i], "-j") == 0) {
            if (i + 1 < argc) {
                g_num_threads = atoi(argv[++i]);
                if (g_num_threads < 1) g_num_threads = 1;
                if (g_num_threads > 64) g_num_threads = 64;
            }
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else {
            printf("unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

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
    printf("database connected\n");
    if (g_num_threads > 1)
        printf("using %d parallel threads\n", g_num_threads);

    // enable fast bulk import mode
    qry("SET autocommit=0");
    qry("SET unique_checks=0");
    qry("SET foreign_key_checks=0");
    printf("bulk import mode enabled\n\n");

    // save old pid mapping before cleaning (for artifact remapping)
    if (do_all || do_players) {
        save_old_pid_mapping();
    }

    // run clean for selected migrations only
    if (do_clean) {
        printf("cleaning selected tables...\n");
        if (do_all || do_accounts) clean_accounts();
        if (do_all || do_players) {
            clean_players();
            clean_frag_leaderboard();
        }
        if (do_all || do_lockers) clean_lockers();
        if (do_all || do_ships) clean_ships();
        if (do_all || do_guilds) clean_guilds();
        if (do_all || do_recipes) clean_recipes();
        if (do_all || do_spellbooks) clean_spellbooks();
        if (do_all || do_shopkeepers) clean_shopkeepers();
        if (do_all) {
            clean_corpses();
            clean_saved_items();
        }
        printf("done.\n\n");
    }

    int accounts = 0, recipes = 0, ships = 0, guilds = 0;
    int corpses = 0, saved_items = 0, players = 0, lockers = 0, spellbooks = 0, shopkeepers = 0;

    // players must be migrated first so account_characters pid lookups work
    if (do_all || do_players) {
        players = migrate_players_from_files();
        remap_artifact_locations();
        remap_ip_info();
        update_player_last_ip();
        cleanup_old_pid_mapping();
        populate_frag_leaderboard();
        printf("\n");
    }
    if (do_frag && !do_all && !do_players) {
        populate_frag_leaderboard();
        printf("\n");
    }
    if (do_all || do_accounts) {
        accounts = migrate_accounts_from_files();
        printf("\n");
    }
    if (do_all || do_recipes) {
        recipes = migrate_recipes_from_files();
        printf("\n");
    }
    if (do_all || do_ships) {
        ships = migrate_ships_from_files();
        printf("\n");
    }
    if (do_all || do_guilds) {
        guilds = migrate_guilds_from_files();
        printf("\n");
    }
    if (do_all) {
        corpses = migrate_corpses_from_files();
        printf("\n");
        saved_items = migrate_saved_items_from_files();
        printf("\n");
    }
    if (do_all || do_lockers) {
        lockers = migrate_lockers_from_files();
        printf("\n");
    }
    if (do_all || do_spellbooks) {
        spellbooks = migrate_spellbooks_from_files();
        printf("\n");
    }
    if (do_all || do_shopkeepers) {
        shopkeepers = migrate_shopkeepers_from_files();
        printf("\n");
    }

    // commit and restore normal mode
    printf("committing transaction...\n");
    qry("COMMIT");
    qry("SET unique_checks=1");
    qry("SET foreign_key_checks=1");
    qry("SET autocommit=1");

    printf("\n=== migration complete ===\n");
    if (do_all) {
        printf("players: %d\n", players);
        printf("accounts: %d\n", accounts);
        printf("recipes: %d\n", recipes);
        printf("ships: %d\n", ships);
        printf("guilds: %d\n", guilds);
        printf("corpses: %d\n", corpses);
        printf("saved items: %d\n", saved_items);
        printf("lockers: %d\n", lockers);
        printf("spellbooks: %d\n", spellbooks);
        printf("shopkeepers: %d\n", shopkeepers);
    }

    return 0;
}
