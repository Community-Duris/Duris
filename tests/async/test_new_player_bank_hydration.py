#!/usr/bin/env python3
"""Execute the first-save hydration block with real domain authority and live siblings.

Account login replaces an existing connection, so the simultaneous same-account
descriptor case is exercised here rather than bypassing login in a socket test.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(text, start):
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[opening:end]


files = (ROOT / "src/core/files.c").read_text()
start = files.index("if (establishing_baseline)")
hydrate = "bool hydrate(P_char ch) " + block(files, start)
hydrate = hydrate[:-1] + "return true;\n}"
utility = (ROOT / "src/core/utility.c").read_text()
start = utility.index("void publish_account_bank_balances_revision(")
publication = utility[start:utility.index("{", start)] + block(utility, start)

PRELUDE = r'''
#include "core/utils.h"
#include "account/account.h"
#include "sql/sql_player.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "item/item_ownership_runtime.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstring>
#include <filesystem>
#include <iostream>
P_desc descriptor_list = nullptr;
static std::string root;
static bool item_read_ok = true, item_hydrate_ok = true;
const char *persistence_mode_flatfile_root() { return root.c_str(); }
const char *get_account_name_safe(P_char ch) { return ch->desc->account->acct_name; }
void gmcp_char_vitals(P_char) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
void statuslog(int, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *,
                       const char *, const char *, ...) {}
flatfile_item_repository_result flatfile_item_repository_load_owner(
    const std::string &, const item_owner_identity &, uint64_t *revision,
    std::vector<flatfile_item_ownership_record> *, std::string *)
{
    *revision = 1;
    return item_read_ok ? flatfile_item_repository_result::ok :
                         flatfile_item_repository_result::io_error;
}
bool item_ownership_runtime_hydrate_owner(const item_owner_identity &, uint64_t)
{ return item_hydrate_ok; }
'''

MAIN = r'''
int main(int argc, char **argv)
{
    assert(argc == 2);
    for (bool populated : {false, true}) {
        root = std::string(argv[1]) + (populated ? "/populated" : "/empty");
        std::filesystem::create_directories(root + "/domains");
        std::filesystem::permissions(root, std::filesystem::perms::owner_all);
        std::filesystem::permissions(root + "/domains", std::filesystem::perms::owner_all);
        std::string error;
        flatfile_player_domain_record initial;
        initial.pid = 1; initial.account_name = "Shared"; initial.racewar = 1;
        if (populated) {
            auto seed = initial; seed.pid = 2; seed.domains.bank = {19, 23, 31, 47};
            assert(flatfile_player_domain_establish(root, seed, &error) ==
                   flatfile_player_domain_result::ok);
        }
        assert(flatfile_player_domain_establish_initial_player(root, initial, &error) ==
               flatfile_player_domain_result::ok);
        pc_only_data pc = {}, sibling_pc = {}, opposite_pc = {};
        pc.pid = 1; sibling_pc.pid = 2;
        char_data actor = {}, sibling = {}, opposite = {};
        actor.only.pc = &pc; sibling.only.pc = &sibling_pc; opposite.only.pc = &opposite_pc;
        actor.player.racewar = sibling.player.racewar = 1; opposite.player.racewar = 2;
        char account_name[] = "Shared";
        acct_entry account = {}; account.acct_name = account_name;
        descriptor_data creating = {}, online = {}, other_side = {};
        creating.character = &actor; creating.account = &account;
        creating.connected = CON_PLAYING + 1; creating.next = &online; actor.desc = &creating;
        online.character = &sibling; online.account = &account; online.connected = CON_PLAYING;
        online.next = &other_side;
        other_side.character = &opposite; other_side.account = &account;
        other_side.connected = CON_PLAYING;
        descriptor_list = &creating;
        GET_BALANCE_COPPER(&actor) = 77; GET_BALANCE_COPPER(&sibling) = 88;
        // Neither failed domain read nor failed item initialization publishes a bank.
        pc.pid = 404; assert(!hydrate(&actor)); pc.pid = 1;
        const auto bank_path = root + "/domains/bank-shared-1.domain";
        std::filesystem::rename(bank_path, bank_path + ".held");
        assert(!hydrate(&actor));
        std::filesystem::rename(bank_path + ".held", bank_path);
        item_read_ok = false; assert(!hydrate(&actor)); item_read_ok = true;
        item_hydrate_ok = false; assert(!hydrate(&actor)); item_hydrate_ok = true;
        assert(pc.bank_revision == 0 && GET_BALANCE_COPPER(&actor) == 77);
        assert(sibling_pc.bank_revision == 0 && GET_BALANCE_COPPER(&sibling) == 88);
        assert(hydrate(&actor));
        assert(pc.bank_revision == 1 && sibling_pc.bank_revision == 1);
        assert(GET_BALANCE_COPPER(&actor) == (populated ? 19 : 0));
        assert(GET_BALANCE_SILVER(&actor) == (populated ? 23 : 0));
        assert(GET_BALANCE_GOLD(&actor) == (populated ? 31 : 0));
        assert(GET_BALANCE_PLATINUM(&actor) == (populated ? 47 : 0));
        assert(GET_BALANCE_COPPER(&sibling) == GET_BALANCE_COPPER(&actor));
        assert(opposite_pc.bank_revision == 0 && GET_BALANCE_COPPER(&opposite) == 0);
        // First-session command uses the actual hydrated revision, and replay cannot mint.
        currency_command_payload payload = {};
        payload.pid = 1; payload.racewar = 1; payload.reason = currency_reason_type::wallet_reward;
        strcpy(payload.account_name.data(), account_name); payload.wallet_delta.amount[0] = 7;
        critical_operation_id id = {}; id.bytes[0] = 201;
        critical_command command;
        assert(currency_command_build(&command, id, payload, pc.wallet_revision, pc.bank_revision,
            critical_source_site::command, critical_deadline_class::interactive));
        command.accepted_at_usec = 1;
        assert(flatfile_player_domain_apply(root, command).outcome == critical_apply_outcome::applied);
        assert(flatfile_player_domain_apply(root, command).outcome == critical_apply_outcome::already_applied);
        flatfile_player_domain_record loaded;
        assert(flatfile_player_domain_load(root, 1, account_name, 1, &loaded, &error) ==
               flatfile_player_domain_result::ok);
        assert(loaded.domains.wallet[0] == 7 && loaded.domains.wallet_revision == 1);
        assert(loaded.domains.bank_revision == 2);
        assert(loaded.domains.bank[0] == static_cast<uint64_t>(populated ? 19 : 0));
        // Hydration retry reads revision two, preserves authority, and refreshes the sibling.
        assert(hydrate(&actor) && pc.bank_revision == 2 && sibling_pc.bank_revision == 2);
        assert(flatfile_player_domain_apply(root, command).outcome == critical_apply_outcome::already_applied);
        assert(flatfile_player_domain_load(root, 1, account_name, 1, &loaded, &error) ==
               flatfile_player_domain_result::ok && loaded.domains.wallet[0] == 7 &&
               loaded.domains.bank_revision == 2);
        sibling_pc.bank_revision = 3; GET_BALANCE_COPPER(&sibling) = 99;
        assert(hydrate(&actor) && sibling_pc.bank_revision == 3 && GET_BALANCE_COPPER(&sibling) == 99);
        auto oversized = initial; oversized.pid = 3; oversized.racewar = 2;
        oversized.domains.bank[0] = static_cast<uint64_t>(INT_MAX) + 1;
        assert(flatfile_player_domain_establish(root, oversized, &error) ==
               flatfile_player_domain_result::ok);
        pc.pid = 3; actor.player.racewar = 2;
        const int before = GET_BALANCE_COPPER(&actor);
        assert(!hydrate(&actor) && pc.bank_revision == 2 && GET_BALANCE_COPPER(&actor) == before);
    }
    std::cout << "new-player bank hydration, failure, online siblings and replay passed\n";
}
'''

with tempfile.TemporaryDirectory(prefix="duris-bank-hydration-") as temporary:
    temporary = Path(temporary)
    source = temporary / "harness.cpp"
    source.write_text("\n".join((PRELUDE, publication, hydrate, MAIN)))
    binary = temporary / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-D__NO_MYSQL__",
        "-Isrc", "-Isrc/no_mysql", str(source),
        "src/flatfile/flatfile_player_domain_repository.c",
        "src/flatfile/flatfile_authority_transaction.c", "src/flatfile/flatfile_store.c",
        "src/world/epic_command.c", "src/economy/currency_command.c",
        "src/combat/combat_outcome_command.c", "src/persistence/critical_command.c",
        "-lcrypto", "-pthread", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(temporary / "state")], check=True)
