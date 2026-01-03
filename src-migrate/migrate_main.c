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
    printf("  --clean      clear relevant tables before migration\n");
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
    qry("DELETE FROM player_data");
}

static void clean_accounts(void) {
    printf("  clearing account tables...\n");
    qry("DELETE FROM account_characters");
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

static void clean_corpses(void) {
    printf("  clearing corpse tables...\n");
    qry("DELETE FROM corpse_items");
    qry("DELETE FROM corpses");
}

static void clean_saved_items(void) {
    printf("  clearing saved_items table...\n");
    qry("DELETE FROM saved_items");
}

int main(int argc, char **argv) {
    // parse args
    int do_all = (argc == 1);
    int do_accounts = 0, do_players = 0, do_lockers = 0, do_ships = 0;
    int do_guilds = 0, do_recipes = 0, do_spellbooks = 0;
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
        else if (strcmp(argv[i], "--clean") == 0) do_clean = 1;
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
    printf("database connected\n\n");

    // run clean for selected migrations only
    if (do_clean) {
        printf("cleaning selected tables...\n");
        if (do_all || do_accounts) clean_accounts();
        if (do_all || do_players) clean_players();
        if (do_all || do_lockers) clean_lockers();
        if (do_all || do_ships) clean_ships();
        if (do_all || do_guilds) clean_guilds();
        if (do_all || do_recipes) clean_recipes();
        if (do_all || do_spellbooks) clean_spellbooks();
        if (do_all) {
            clean_corpses();
            clean_saved_items();
        }
        printf("done.\n\n");
    }

    int accounts = 0, recipes = 0, ships = 0, guilds = 0;
    int corpses = 0, saved_items = 0, players = 0, lockers = 0, spellbooks = 0;

    // players must be migrated first so account_characters pid lookups work
    if (do_all || do_players) {
        players = migrate_players_from_files();
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
    }

    return 0;
}
