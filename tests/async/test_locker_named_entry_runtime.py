#!/usr/bin/env python3
"""Execute named-locker resolution and exact account-owner admission.

SQL responses are controlled here; the MariaDB gameplay journey exercises the
real query and grants. Flat-file identities use the real persisted repository.
"""
from pathlib import Path
import os
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function, rel

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "persistence/persistence_mode.h"
#include "flatfile/flatfile_identity_repository.h"
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
using MYSQL_RES = int;
using MYSQL_ROW = char **;
static int database, result;
static int *DB = &database;
static persistence_mode mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
static std::string root, account = "Owneracct";
static bool present = true, query_error = false, escape_error = false;
static char sql_account[] = "Owneracct", sql_side[] = "1";
static char *row[] = {sql_account, sql_side};
static char display_character[] = "Firstchar";
static char *display_row[] = {display_character};
static bool display_query = false;
static int reads = 0;
persistence_mode persistence_mode_get() { return mode; }
const char *persistence_mode_flatfile_root() { return root.empty() ? nullptr : root.c_str(); }
const char *get_account_name_safe(P_char) { return account.c_str(); }
char *sql_escape_string(const char *value) { return escape_error ? nullptr : strdup(value); }
MYSQL_RES *db_query(const char *format, ...) {
    assert(strstr(format, "blocked=0") && strstr(format, "deleted_at IS NULL"));
    assert(strstr(format, "LIMIT 1"));
    display_query = strstr(format, "SELECT char_name") != nullptr;
    if (display_query) assert(strstr(format, "racewar=%d") && strstr(format, "ORDER BY last_login DESC, pid ASC"));
    ++reads;
    return query_error ? nullptr : &result;
}
MYSQL_ROW mysql_fetch_row(MYSQL_RES *) { return present ? (display_query ? display_row : row) : nullptr; }
void mysql_free_result(MYSQL_RES *) {}
'''

DRIVER = r'''
static void resolves(const char *name, const char *expected) {
    std::string target;
    assert(resolve_named_locker(name, &target));
    assert(target == expected);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    resolves("VaUlToWnEr", "account.owneracct.1.locker");
    // Character aliases always reach the same account storage.
    resolves("VaUlToWnEr.LoCkEr", "account.owneracct.1.locker");
    const int before = reads;
    resolves("AcCoUnT.Owneracct.1", "account.owneracct.1.locker");
    resolves("AcCoUnT.Owneracct.1.LoCkEr", "account.owneracct.1.locker");
    resolves("guild.17", "guild.17.locker");
    assert(reads == before);
    present = false;
    std::string target;
    assert(!resolve_named_locker("Oldowner", &target));
    assert(!resolve_named_locker("Oldowner.locker", &target));
    query_error = true;
    assert(!resolve_named_locker("Vaultowner", &target));
    query_error = false;
    escape_error = true;
    assert(!resolve_named_locker("Vaultowner", &target));
    escape_error = false;
    DB = nullptr;
    assert(!resolve_named_locker("Vaultowner", &target));
    DB = &database;
    present = true;
    row[0] = nullptr;
    assert(!resolve_named_locker("Vaultowner", &target));
    row[0] = sql_account;
    char_data owner{};
    GET_RACEWAR(&owner) = 1;
    assert(locker_is_account_owner(&owner, "account.owneracct.1.locker"));
    for (const char *denied : {"account.owneracct.2.locker", "account.owneracct.1junk.locker",
                              "account.other.1.locker", "vaultowner.locker", "guild.1.locker"})
        assert(!locker_is_account_owner(&owner, denied));
    account = "Unknown";
    assert(!locker_is_account_owner(&owner, "account.unknown.1.locker"));
    account = "Owneracct";
    for (const char *invalid : {"account.owneracct.1junk", "account.owneracct.2.more", "guild.0", "guild.2junk", "missing.other", ".locker"})
        assert(!resolve_named_locker(invalid, &target));
    char_data storage{};
    char own_name[] = "vAuLtOwNeR", canonical[] = "account.owneracct.1.locker";
    owner.player.name = own_name;
    storage.player.name = canonical;
    GET_RACEWAR(&storage) = 1;
    assert(locker_display_name(&owner, &storage, "Altchar") == "Vaultowner");
    account = "Visitoracct";
    assert(locker_display_name(&owner, &storage, "aLtChAr") == "Altchar");
    assert(locker_display_name(&owner, &storage, nullptr) == "Firstchar");
    query_error = true;
    assert(locker_display_name(&owner, &storage, nullptr).empty());
    query_error = false;
    puts("PASS SQL account-only routing, aliases, failures, exact ownership and character-only titles");

    mode = PERSISTENCE_MODE_FLATFILE_PRIMARY;
    assert(!resolve_named_locker("Vaultowner", &target));
    root = argv[1];
    for (const auto &path : {root, root + "/identities", root + "/identities/names", root + "/domains"}) {
        std::filesystem::create_directories(path);
        chmod(path.c_str(), 0700);
    }
    int32_t pid = 0;
    std::string error;
    assert(flatfile_identity_allocate_pid(root, &pid, &error) == flatfile_identity_result::ok);
    assert(flatfile_identity_claim(root, pid, "Vaultowner", "Owneracct", &error) == flatfile_identity_result::ok);
    flatfile_identity_record identity;
    assert(flatfile_identity_lookup_pid(root, pid, &identity, &error) == flatfile_identity_result::ok);
    identity.racewar = 1;
    assert(flatfile_identity_sync_account(root, "Owneracct", {identity}, &error) == flatfile_identity_result::ok);
    resolves("VaUlToWnEr", "account.owneracct.1.locker");
    resolves("vaultowner.locker", "account.owneracct.1.locker");
    assert(locker_display_name(&owner, &storage, nullptr) == "Vaultowner");
    int32_t alt_pid = 0;
    assert(flatfile_identity_allocate_pid(root, &alt_pid, &error) == flatfile_identity_result::ok);
    assert(flatfile_identity_claim(root, alt_pid, "Altchar", "Owneracct", &error) == flatfile_identity_result::ok);
    flatfile_identity_record alt;
    assert(flatfile_identity_lookup_pid(root, alt_pid, &alt, &error) == flatfile_identity_result::ok);
    alt.racewar = 1; alt.last_login = 100;
    assert(flatfile_identity_sync_account(root, "Owneracct", {identity, alt}, &error) == flatfile_identity_result::ok);
    assert(locker_display_name(&owner, &storage, nullptr) == "Altchar");
    alt.racewar = 2;
    assert(flatfile_identity_sync_account(root, "Owneracct", {identity, alt}, &error) == flatfile_identity_result::ok);
    assert(locker_display_name(&owner, &storage, nullptr) == "Vaultowner");
    assert(flatfile_identity_set_blocked(root, pid, true, &error) == flatfile_identity_result::ok);
    assert(!resolve_named_locker("Vaultowner", &target));
    assert(flatfile_identity_remove(root, pid, "Vaultowner", &error) == flatfile_identity_result::ok);
    assert(!resolve_named_locker("Vaultowner", &target));
    assert(!resolve_named_locker("Missing", &target));
    std::ofstream(root + "/identities/names/catalog.identity", std::ios::trunc) << "corrupt";
    assert(!resolve_named_locker("Vaultowner", &target));
    assert(!resolve_named_locker("vaultowner.locker", &target));
    assert(locker_display_name(&owner, &storage, nullptr).empty());
    puts("PASS real flat-file identity catalog: mixed case, title ordering/faction/privacy, blocked/deleted/missing/corrupt refusal");
}
'''

source = (SRC / 'storage_lockers.c').read_text()
assert 'resolve_named_locker(enterWho, &target)' in source
assert 'locker_is_account_owner(ch, target)' in source
# The menu and title both order by actual login time. SQL timestamps must reach
# the account's integer time field as epoch seconds, not the year prefix.
assert 'UNIX_TIMESTAMP(ac.last_login)' in (SRC / 'sql_player.c').read_text()
assert 'ORDER BY ac.last_login DESC, ac.pid ASC' in (SRC / 'sql_player.c').read_text()
with tempfile.TemporaryDirectory(prefix='locker-name-') as directory:
    harness, binary = Path(directory) / 'harness.cpp', Path(directory) / 'harness'
    harness.write_text(PRELUDE + '\n' +
                       extract_function('storage_lockers.c', 'static bool resolve_named_locker(') + '\n' +
                       extract_function('storage_lockers.c', 'static bool locker_is_account_owner(') + '\n' +
                       extract_function('storage_lockers.c', 'static std::string locker_display_name(') + '\n' + DRIVER)
    subprocess.run(['g++', '-std=c++20', '-g', '-O1', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                    '-Isrc', str(harness), rel('flatfile_identity_repository.c'),
                    rel('flatfile_authority_transaction.c'), rel('flatfile_store.c'),
                    '-lcrypto', '-pthread', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(Path(directory) / 'state')], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
                        'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'})
