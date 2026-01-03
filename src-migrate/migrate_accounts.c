// migrate_accounts.c
// account migration for pfile migration tool

#include "migrate_common.h"

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

// count account files for progress bar
static int count_account_files(void) {
    int total = 0;
    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Accounts/%c", letter);
        DIR *dir = opendir(dirname);
        if (!dir) continue;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (strstr(entry->d_name, ".bak")) continue;
            if (strstr(entry->d_name, ".backup")) continue;
            if (strstr(entry->d_name, ".old")) continue;
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

int migrate_accounts_from_files(void) {
    int count = 0;
    int errors = 0;
    int processed = 0;

    int total = count_account_files();
    struct progress_bar pb;
    progress_init(&pb, total, "accounts");

    for (char letter = 'a'; letter <= 'z'; letter++) {
        char dirname[256];
        snprintf(dirname, sizeof(dirname), "Accounts/%c", letter);

        DIR *dir = opendir(dirname);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (strstr(entry->d_name, ".bak")) continue;
            if (strstr(entry->d_name, ".backup")) continue;
            if (strstr(entry->d_name, ".old")) continue;

            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);

            struct stat st;
            if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode)) continue;

            FILE *f = fopen(filepath, "r");
            if (!f) {
                errors++;
                processed++;
                progress_update(&pb, processed);
                continue;
            }

            struct acct_entry *acct = (struct acct_entry *)malloc(sizeof(struct acct_entry));
            memset(acct, 0, sizeof(struct acct_entry));

            char line[4096];
            int serial = 0;

            fscanf(f, "%d\n", &serial);

            fgets(line, sizeof(line), f);
            line[strcspn(line, "\r\n")] = 0;
            acct->acct_name = str_dup(line);

            fgets(line, sizeof(line), f);
            line[strcspn(line, "\r\n")] = 0;
            acct->acct_email = str_dup(line);

            fgets(line, sizeof(line), f);
            line[strcspn(line, "\r\n")] = 0;
            acct->acct_password = str_dup(line);

            fgets(line, sizeof(line), f);
            line[strcspn(line, "\r\n")] = 0;
            acct->acct_confirmation = str_dup(line);

            read_unique_ip_file(acct, f);
            read_character_list_file(acct, f);

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

            if (sql_save_account(acct)) {
                count++;
            } else {
                errors++;
            }

            free_account(acct);
            processed++;
            progress_update(&pb, processed);
        }

        closedir(dir);
    }

    progress_finish(&pb);
    printf("accounts: %d migrated, %d errors\n", count, errors);
    return count;
}
