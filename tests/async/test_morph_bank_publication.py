#!/usr/bin/env python3
"""Exercise the production shared-bank publisher for a morphed player."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for end in range(opening, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[start : end + 1]
    raise AssertionError("unterminated bank publication function")


PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <strings.h>

constexpr int CON_PLAYING = 0;
struct AccountBankBalances {
    int copper, silver, gold, platinum;
};
struct pc_only_data {
    uint64_t bank_revision = 1;
};
struct character {
    bool npc = false;
    int racewar = 1;
    character *original = nullptr;
    struct { pc_only_data *pc = nullptr; } only;
    AccountBankBalances balance = {};
};
using P_char = character *;
struct acct_record {
    const char *acct_name;
};
struct descriptor {
    int connected = CON_PLAYING;
    P_char character = nullptr;
    P_char original = nullptr;
    acct_record *account = nullptr;
    descriptor *next = nullptr;
};
using P_desc = descriptor *;
P_desc descriptor_list = nullptr;
int gmcp_calls = 0;
void gmcp_char_vitals(P_char) { ++gmcp_calls; }

#define IS_NPC(ch) ((ch)->npc)
#define IS_MORPH(ch) ((ch)->npc && (ch)->original)
#define MORPH_ORIG(ch) ((ch)->original)
#define GET_RACEWAR(ch) ((ch)->racewar)
#define GET_BALANCE_COPPER(ch) ((ch)->balance.copper)
#define GET_BALANCE_SILVER(ch) ((ch)->balance.silver)
#define GET_BALANCE_GOLD(ch) ((ch)->balance.gold)
#define GET_BALANCE_PLATINUM(ch) ((ch)->balance.platinum)
'''

DRIVER = r'''
int main()
{
    character direct, original, morph;
    pc_only_data direct_pc, original_pc;
    direct.only.pc = &direct_pc;
    original.only.pc = &original_pc;
    morph.npc = true;
    morph.original = &original;
    acct_record shared{"shared"};
    descriptor morphed;
    morphed.character = &morph;
    morphed.account = &shared;
    descriptor ordinary;
    ordinary.character = &direct;
    ordinary.account = &shared;
    ordinary.next = &morphed;
    descriptor_list = &ordinary;

    const AccountBankBalances committed = {11, 12, 13, 14};
    publish_account_bank_balances_revision("shared", 1, &committed, 2);
    assert(direct.balance.copper == 11 && original.balance.copper == 11);
    assert(direct.balance.platinum == 14 && original.balance.platinum == 14);
    assert(direct_pc.bank_revision == 2 && original_pc.bank_revision == 2);
    assert(morph.balance.copper == 0);
    assert(gmcp_calls == 2);

    const AccountBankBalances stale = {99, 99, 99, 99};
    publish_account_bank_balances_revision("shared", 1, &stale, 1);
    assert(direct.balance.copper == 11 && original.balance.copper == 11);
    assert(direct_pc.bank_revision == 2 && original_pc.bank_revision == 2);
    assert(gmcp_calls == 2);

    morphed.original = &morph;
    const AccountBankBalances newer = {21, 22, 23, 24};
    publish_account_bank_balances_revision("shared", 1, &newer, 3);
    assert(direct.balance.copper == 21 && original.balance.copper == 21);
    assert(direct_pc.bank_revision == 3 && original_pc.bank_revision == 3);
    assert(gmcp_calls == 4);
}
'''


def main() -> None:
    source = (ROOT / "src/core/utility.c").read_text(encoding="utf-8")
    function = extract_function(source, "void publish_account_bank_balances_revision(")
    with tempfile.TemporaryDirectory(prefix="duris-morph-bank-") as directory:
        path = Path(directory) / "morph_bank.cpp"
        binary = Path(directory) / "morph_bank"
        path.write_text("\n".join((PRELUDE, function, DRIVER)), encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        str(path), "-o", str(binary)], check=True, cwd=ROOT)
        subprocess.run([str(binary)], check=True, cwd=ROOT)
    print("Morph shared-bank publication and revision ordering passed.")


if __name__ == "__main__":
    main()
