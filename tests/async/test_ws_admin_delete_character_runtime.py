#!/usr/bin/env python3
"""Execute the production web-admin deletion handler with isolated store stubs."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for end in range(brace, len(source)):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
            if depth == 0:
                return source[start : end + 1]
    raise AssertionError(f"unterminated function: {signature}")


handlers = (ROOT / "src/net/ws_handlers.c").read_text()
cleanup_signature = "static void ws_free_admin_delete_temp_character("
cleanup = function(handlers, cleanup_signature) if cleanup_signature in handlers else ""
handler = function(handlers, "void ws_cmd_admin_delete_character(")

prelude = r'''
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

static int allocations_freed = 0;
static void fixture_free(void *p) { if (p) { ++allocations_freed; std::free(p); } }
#define free fixture_free
#define FREE(p) fixture_free(p)
#define LOG_PLAYER 1
#define AVATAR 56
#define PERSISTENCE_MODE_MARIADB_PRIMARY 0
#define PERSISTENCE_MODE_MARIADB_PRIMARY_FLATFILE_FALLBACK 1
#define PERSISTENCE_MODE_FLATFILE_PRIMARY 2

struct cJSON {
    enum kind_t { object, string, number } kind = object;
    char *valuestring = nullptr;
    int valueint = 0;
    std::map<std::string, cJSON *> fields;
};
static bool cJSON_IsObject(const cJSON *value) { return value && value->kind == cJSON::object; }
static bool cJSON_IsString(const cJSON *value) { return value && value->kind == cJSON::string; }
static bool cJSON_IsNumber(const cJSON *value) { return value && value->kind == cJSON::number; }
static cJSON *cJSON_GetObjectItem(cJSON *value, const char *name) {
    if (!value) return nullptr;
    auto found = value->fields.find(name);
    return found == value->fields.end() ? nullptr : found->second;
}

struct acct_chars { char *charname; acct_chars *next; };
struct account_data { char *acct_name; acct_chars *acct_character_list; int num_chars; };
struct pc_only_data { char *poofIn; char *poofOut; char **gcmd_arr; };
struct char_data {
    struct { char *name; char *title; char *short_descr; char *long_descr; char *description; } player;
    struct { pc_only_data *pc; } only;
};
struct descriptor_data { int durisweb_verified; };
using P_char = char_data *;
using P_acct = account_data *;
enum class character_delete_result { deleted, refused, reconciliation_required };
void ws_cmd_admin_delete_character(descriptor_data *, cJSON *);

struct Response { int success; std::string account, name, request_id, error; };
struct Progress { std::string request_id, message, level; };
static Response response{};
static std::vector<Progress> progress;
static int persistence_mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
static int restore_result = 0;
static character_delete_result core_result = character_delete_result::deleted;
static bool hook_enabled = true;
static int hook_calls = 0, allocate_calls = 0, read_calls = 0, restore_calls = 0;
static int soft_delete_calls = 0, write_calls = 0, typed_delete_calls = 0, legacy_delete_calls = 0;
static int string_free_calls = 0, deletion_log_calls = 0, durable_account_entries = 1;

static char *duplicate(const char *value) {
    const size_t length = std::strlen(value) + 1;
    auto *copy = static_cast<char *>(std::malloc(length));
    std::memcpy(copy, value, length);
    return copy;
}
void str_free(char *p) { ++string_free_calls; fixture_free(p); }
char *str_dup(const char *p) { return duplicate(p); }
void ws_send_admin_delete_progress(descriptor_data *, const char *request_id,
                                   const char *message, const char *level) {
    progress.push_back({request_id ? request_id : "", message ? message : "", level ? level : ""});
}
void ws_send_admin_delete_response(descriptor_data *, int success, const char *account,
                                   const char *name, const char *request_id, const char *error) {
    response = {success, account ? account : "", name ? name : "",
                request_id ? request_id : "", error ? error : ""};
}
bool durisweb_hook_enabled(const char *) { ++hook_calls; return hook_enabled; }
P_acct allocate_account() { ++allocate_calls; return new account_data{}; }
int read_account(P_acct account) {
    ++read_calls;
    account->acct_character_list = nullptr;
    account->num_chars = 0;
    if (durable_account_entries) {
        auto *entry = static_cast<acct_chars *>(std::calloc(1, sizeof(acct_chars)));
        entry->charname = duplicate("Fixture");
        account->acct_character_list = entry;
        account->num_chars = 1;
    }
    return 1;
}
P_acct free_account(P_acct account) {
    if (!account) return nullptr;
    while (account->acct_character_list) {
        auto *entry = account->acct_character_list;
        account->acct_character_list = entry->next;
        fixture_free(entry->charname);
        fixture_free(entry);
    }
    fixture_free(account->acct_name);
    delete account;
    return nullptr;
}
int write_account(P_acct account) {
    ++write_calls;
    durable_account_entries = account->num_chars;
    return 1;
}
int restoreCharOnly(P_char character, char *) {
    ++restore_calls;
    character->player.name = duplicate("Fixture");
    character->player.title = duplicate("title");
    character->player.short_descr = duplicate("short");
    character->player.long_descr = duplicate("long");
    character->player.description = duplicate("description");
    return restore_result;
}
bool sql_soft_delete_character(long) { ++soft_delete_calls; return true; }
int persistence_mode_get() { return persistence_mode; }
void logit(int type, const char *, ...) { if (type == LOG_PLAYER) ++deletion_log_calls; }
void statuslog(int, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *,
                       const char *, const char *, ...) {}
int deleteCharacter(P_char) { ++legacy_delete_calls; return 1; }
character_delete_result delete_character_result(P_char, bool = true) {
    ++typed_delete_calls;
    return core_result;
}

static cJSON make_string(char *value) { cJSON result{}; result.kind = cJSON::string; result.valuestring = value; return result; }
static cJSON make_number(int value) { cJSON result{}; result.kind = cJSON::number; result.valueint = value; return result; }
static cJSON make_object() { cJSON result{}; result.kind = cJSON::object; return result; }
static void reset(int mode, int restored, character_delete_result deleted, bool hook = true) {
    response = {};
    progress.clear();
    persistence_mode = mode;
    restore_result = restored;
    core_result = deleted;
    hook_enabled = hook;
    hook_calls = allocate_calls = read_calls = restore_calls = 0;
    soft_delete_calls = write_calls = typed_delete_calls = legacy_delete_calls = 0;
    string_free_calls = deletion_log_calls = allocations_freed = 0;
    durable_account_entries = 1;
}
static void invoke(bool authorized = true) {
    cJSON request = make_string(const_cast<char *>("delete-req-42"));
    cJSON account = make_string(const_cast<char *>("TestAccount"));
    cJSON name = make_string(const_cast<char *>("Fixture"));
    cJSON pid = make_number(741);
    cJSON deleted_by = make_string(const_cast<char *>("operator"));
    cJSON data = make_object();
    data.fields["requestId"] = &request;
    data.fields["account"] = &account;
    data.fields["name"] = &name;
    data.fields["pid"] = &pid;
    data.fields["deletedBy"] = &deleted_by;
    descriptor_data descriptor{authorized ? 1 : 0};
    ws_cmd_admin_delete_character(&descriptor, &data);
}
static bool deletion_success_progress_seen() {
    for (const auto &item : progress)
        if (item.message == "Character save file deleted" ||
            item.message == "Character deletion completed" || item.message == "Account file updated")
            return true;
    return false;
}
static bool correlated_error_progress_seen() {
    for (const auto &item : progress)
        if (item.level == "error" && item.request_id == "delete-req-42") return true;
    return false;
}

'''

main = r'''
int main() {
    // Both restore errors must defer cleanup in every SQL-primary mode.
    for (int mode : {PERSISTENCE_MODE_MARIADB_PRIMARY,
                     PERSISTENCE_MODE_MARIADB_PRIMARY_FLATFILE_FALLBACK}) {
        for (int failure : {-1, -2}) {
            reset(mode, failure, character_delete_result::deleted);
            invoke();
            assert(soft_delete_calls == 0 && write_calls == 0);
            assert(durable_account_entries == 1 && typed_delete_calls == 0);
            assert(response.success == 0 && response.request_id == "delete-req-42");
            assert(!response.error.empty() && !deletion_success_progress_seen());
            assert(correlated_error_progress_seen() && deletion_log_calls == 0);
            assert(string_free_calls == 5);
        }
    }

    // Refusal and uncertain reconciliation from the core gate are not success.
    for (auto result : {character_delete_result::refused,
                        character_delete_result::reconciliation_required}) {
        reset(PERSISTENCE_MODE_MARIADB_PRIMARY, 0, result);
        invoke();
        assert(typed_delete_calls == 1 && legacy_delete_calls == 0);
        assert(soft_delete_calls == 0 && write_calls == 0 && durable_account_entries == 1);
        assert(response.success == 0 && response.request_id == "delete-req-42");
        assert(!response.error.empty() && !deletion_success_progress_seen());
        assert(correlated_error_progress_seen());
        assert(deletion_log_calls == 0 && string_free_calls == 5);
        // The core can return reconciliation_required after a lost COMMIT
        // reply or after committed SQL cleanup. Do not promise rollback.
        if (result == character_delete_result::reconciliation_required) {
            assert(response.error.find("requires reconciliation") != std::string::npos);
            assert(response.error.find("account data was not changed") == std::string::npos);
        }
    }

    // Confirmed core deletion still removes the account projection and reports success.
    reset(PERSISTENCE_MODE_MARIADB_PRIMARY, 0, character_delete_result::deleted);
    invoke();
    assert(typed_delete_calls == 1 && legacy_delete_calls == 0);
    assert(soft_delete_calls == 0 && write_calls == 1 && durable_account_entries == 0);
    assert(response.success == 1 && response.request_id == "delete-req-42");
    assert(response.error.empty() && deletion_success_progress_seen());
    assert(deletion_log_calls == 1 && string_free_calls == 5);

    // Flatfile-primary retains the legacy missing-save compatibility path.
    reset(PERSISTENCE_MODE_FLATFILE_PRIMARY, -1, character_delete_result::deleted);
    invoke();
    assert(soft_delete_calls == 1 && write_calls == 1 && durable_account_entries == 0);
    assert(response.success == 1 && response.request_id == "delete-req-42");
    assert(string_free_calls == 5);

    // Authentication and hook refusals remain ahead of account/player access.
    reset(PERSISTENCE_MODE_MARIADB_PRIMARY, 0, character_delete_result::deleted);
    invoke(false);
    assert(response.success == 0 && response.error == "Not authorized");
    assert(hook_calls == 0 && allocate_calls == 0 && read_calls == 0 && restore_calls == 0);
    reset(PERSISTENCE_MODE_MARIADB_PRIMARY, 0, character_delete_result::deleted, false);
    invoke();
    assert(response.success == 0 && response.error.find("hook is disabled") != std::string::npos);
    assert(response.request_id == "delete-req-42");
    assert(hook_calls == 1 && allocate_calls == 0 && read_calls == 0 && restore_calls == 0);

    std::puts("PASS: actual web-admin character deletion handler result gates");
}
'''

with tempfile.TemporaryDirectory(prefix="ws-admin-delete-") as temporary:
    cpp = Path(temporary) / "runtime.cpp"
    cpp.write_text(prelude + cleanup + "\n" + handler + "\n" + main)
    executable = Path(temporary) / "runtime"
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
         "-fno-omit-frame-pointer", str(cpp), "-o", str(executable)],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
