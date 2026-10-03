#!/usr/bin/env python3
"""Execute the SQL account confirmation owner at the admission/fence boundary.

The actual production function is compiled with injected authority failures.
This complements real native SQL journeys; it does not qualify typed erasure.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/account/account.c").read_text(encoding="utf-8")
start = source.index("void verify_delete_account(")
opening = source.index("{", start)
depth = 0
for end in range(opening, len(source)):
    depth += (source[end] == "{") - (source[end] == "}")
    if depth == 0:
        body = source[start:end + 1]
        break
else:
    raise AssertionError("unterminated account confirmation owner")

prelude = r'''
#include <cctype>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
struct MYSQL {};
MYSQL connection;
MYSQL *DB = &connection;
struct acct_entry { char *acct_name; char acct_blocked; };
using P_acct = acct_entry *;
struct descriptor_data { P_acct account; int state; };
using P_desc = descriptor_data *;
struct account_deletion_identity { int pid = 1; std::string name = "Fixture"; };
constexpr char ACCOUNT_BLOCK_DELETION = 2;
constexpr int CON_DISPLAY_ACCT_MENU = 1, CON_FLUSH = 2, AVATAR = 3;
#define STATE(d) ((d)->state)
bool denied = false, transaction_active = false, fence_ok = true, backend_ok = false;
int leases = 0, admissions = 0, writes = 0, closes = 0, captures = 0, removals = 0, frees = 0;
char durable_block = 1;
std::string messages;
void require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
struct economic_sql_currency_writer_guard {
    bool held = false;
    ~economic_sql_currency_writer_guard() { if (held) --leases; }
    static unsigned int acquire(MYSQL *db, economic_sql_currency_writer_guard *out) noexcept {
        ++admissions;
        if (denied || db != DB) return EAGAIN;
        out->held = true; ++leases; return 0;
    }
};
bool sql_in_transaction() { return transaction_active; }
void SEND_TO_Q(const char *text, P_desc) { messages += text; }
void display_account_deletion_confirmation(P_desc, bool) {}
void display_account_menu(P_desc, char *) {}
template<class... T> void statuslog(int, const char *, T...) {}
template<class... T> void persistence_alert(T...) {}
int write_account(P_acct account) {
    require(leases == 1, "account fence was written without held native admission");
    ++writes;
    if (!fence_ok) return -1;
    durable_block = account->acct_blocked; return 1;
}
bool capture_account_deletion_identities(P_desc, std::vector<account_deletion_identity> *) {
    ++captures; return true;
}
bool account_deletion_locker_runtime_active(const std::string &,
                                          const std::vector<account_deletion_identity> &) { return false; }
void close_other_account_sessions(P_desc) { ++closes; }
struct account_deletion_drain_guard {
    bool drain() { require(leases == 0, "initial admission was retained across worker drain"); return true; }
};
void flush_pending_ship_saves() {}
bool drain_pending_ship_saves() { return true; }
bool sql_delete_account(const char *) { return backend_ok; }
void remove_deleted_account_runtime(P_desc, const std::vector<account_deletion_identity> &) { ++removals; }
void account_recovery_forget(const char *) {}
P_acct free_account(P_acct) { ++frees; return nullptr; }
'''

scenarios = r'''
int main() {
    char name[] = "FixtureAccount";
    acct_entry account{name, 1};
    descriptor_data descriptor{&account, 0};
    auto reset = [&] {
        require(leases == 0, "native lease leaked after confirmation");
        account.acct_blocked = durable_block = 1;
        descriptor.account = &account; descriptor.state = 0;
        denied = transaction_active = false; fence_ok = true; backend_ok = false;
        admissions = writes = closes = captures = removals = frees = 0; messages.clear();
    };
    reset();
    denied = true;
    verify_delete_account(&descriptor, name);
    require(account.acct_blocked == 1 && durable_block == 1 && writes == 0,
            "native admission refusal established the permanent account fence");
    require(admissions == 1 && closes == 0 && captures == 0 && removals == 0 && frees == 0,
            "admission refusal reached account cleanup or session publication");
    require(descriptor.state == CON_DISPLAY_ACCT_MENU && leases == 0 &&
            messages.find("no deletion fence was written") != std::string::npos,
            "admission refusal did not preserve usable account-menu state");
    reset(); transaction_active = true;
    verify_delete_account(&descriptor, name);
    require(admissions == 0 && writes == 0 && account.acct_blocked == 1 && leases == 0,
            "outer transaction bypassed pre-fence admission");
    reset(); fence_ok = false;
    verify_delete_account(&descriptor, name);
    require(admissions == 1 && writes == 1 && account.acct_blocked == 1 && durable_block == 1 &&
            closes == 0 && captures == 0 && leases == 0,
            "fence-write failure changed account/session state or leaked admission");
    reset();
    verify_delete_account(&descriptor, name);
    require(admissions == 1 && writes == 1 && durable_block == ACCOUNT_BLOCK_DELETION &&
            account.acct_blocked == ACCOUNT_BLOCK_DELETION && captures == 1 && closes == 1 &&
            removals == 0 && frees == 0 && leases == 0,
            "admitted but unfinished deletion lost its existing durable fence");
    admissions = writes = 0; denied = true;
    verify_delete_account(&descriptor, name);
    require(admissions == 0 && writes == 0 && durable_block == ACCOUNT_BLOCK_DELETION,
            "already-fenced retry rewrote or cancelled its durable fence");
    char cancel[] = "cancel";
    verify_delete_account(&descriptor, cancel);
    require(account.acct_blocked == ACCOUNT_BLOCK_DELETION &&
            messages.find("cannot be cancelled") != std::string::npos,
            "existing deletion fence became cancellable");
    reset(); backend_ok = true;
    verify_delete_account(&descriptor, name);
    require(admissions == 1 && writes == 1 && captures == 1 && closes == 1 &&
            removals == 1 && frees == 1 && !descriptor.account &&
            descriptor.state == CON_FLUSH && leases == 0,
            "confirmed deletion did not publish cleanup exactly once");
    reset();
    verify_delete_account(&descriptor, cancel);
    require(admissions == 0 && writes == 0 && account.acct_blocked == 1,
            "unfenced cancellation acquired deletion authority");
    char mismatch[] = "fixtureaccount";
    verify_delete_account(&descriptor, mismatch);
    require(admissions == 0 && writes == 0 && account.acct_blocked == 1,
            "wrong-case confirmation acquired deletion authority");
    std::cout << "PASS: SQL account confirmation admission/fence failure, retained retry and publication\n";
}
'''

with tempfile.TemporaryDirectory(prefix="account-delete-admission-") as temporary:
    path = Path(temporary)
    cpp = path / "fixture.cpp"
    binary = path / "fixture"
    cpp.write_text(prelude + body + scenarios, encoding="utf-8")
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", str(cpp), "-o", str(binary)], check=True)
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    subprocess.run([str(binary)], env=environment, check=True)
