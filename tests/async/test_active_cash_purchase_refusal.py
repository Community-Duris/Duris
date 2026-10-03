#!/usr/bin/env python3
"""Exercise stat-shop active refusal and pin nearby unsupported cost gates."""
from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


def function_body(path: str, signature: str) -> str:
    source = (ROOT / path).read_text(encoding="utf-8")
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for end in range(brace, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[start : end + 1]
    raise AssertionError(f"unterminated function: {signature}")


PRELUDE = r'''
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

constexpr int MAX_INPUT_LENGTH = 256;
constexpr int CMD_SET_PERIODIC = 1;
constexpr int CMD_LIST = 2;
constexpr int CMD_BUY = 3;
constexpr int SPELL_TYPE_SPELL = 4;
constexpr int TRUE = 1;
constexpr int FALSE = 0;

struct stats { int Str, Agi, Dex, Con, Luk, Pow, Int, Wis, Cha; };
struct character { stats base_stats = {1,1,1,1,1,1,1,1,1}; int money = 100; };
using P_char = character *;

bool active_mode = false;
int debit_calls = 0;
int spell_calls = 0;
std::string last_message;
class economic_gameplay_authority {
public:
    static bool active() { return active_mode; }
};
void send_to_char(const char *message, P_char) { last_message = message; }
const char *coin_stringv(int) { return "eight coins"; }
char *one_argument(char *input, char *output) {
    while (*input == ' ') ++input;
    while (*input && *input != ' ') *output++ = *input++;
    *output = 0;
    return input;
}
#define GET_MONEY(ch) ((ch)->money)
int SUB_MONEY(P_char ch, int cost, int) {
    ++debit_calls;
    if (active_mode || ch->money < cost) return -1;
    ch->money -= cost;
    return 0;
}
#define SPELL(name, field) \
void spell_perm_increase_##name(int, P_char ch, void *, int, P_char, int) { \
    ++spell_calls; ++ch->base_stats.field; \
}
SPELL(str, Str) SPELL(agi, Agi) SPELL(dex, Dex)
SPELL(con, Con) SPELL(luck, Luk) SPELL(pow, Pow)
SPELL(int, Int) SPELL(wis, Wis) SPELL(cha, Cha)
'''

CHECKS = r'''
int main() {
    for (int choice = 1; choice <= 9; ++choice) {
        character ch;
        char argument[16];
        std::snprintf(argument, sizeof(argument), "%d", choice);
        active_mode = true;
        debit_calls = spell_calls = 0;
        assert(stat_shops(0, &ch, CMD_BUY, argument) == TRUE);
        assert(ch.money == 100);
        assert(debit_calls == 0 && spell_calls == 0);
        assert(last_message.find("unavailable") != std::string::npos);
        for (int stat : {ch.base_stats.Str, ch.base_stats.Agi, ch.base_stats.Dex,
                         ch.base_stats.Con, ch.base_stats.Luk, ch.base_stats.Pow,
                         ch.base_stats.Int, ch.base_stats.Wis, ch.base_stats.Cha})
            assert(stat == 1);

        active_mode = false;
        assert(stat_shops(0, &ch, CMD_BUY, argument) == TRUE);
        assert(ch.money == 92 && debit_calls == 1 && spell_calls == 1);
    }
    character ch;
    active_mode = true;
    debit_calls = spell_calls = 0;
    assert(stat_shops(0, &ch, CMD_LIST, nullptr) == TRUE);
    assert(debit_calls == 0 && spell_calls == 0 && ch.money == 100);
}
'''

EPIC_PRELUDE = r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

struct epic_command_result {};
enum class epic_reason_type { epic_skill_refund };
enum class critical_source_site { recovery };
enum class critical_deadline_class { recovery };
struct skill_state { int taught = 20; int learned = 20; };
struct pc_state { skill_state skills[1]; };
struct character { int money = 100; pc_state *pc = nullptr;
                   struct { pc_state *pc; } only = {}; };
struct Skill { const char *name = "training"; };
using P_char = character *;
Skill skills[1];
constexpr int LOG_WIZ = 1;
constexpr int PULSE_VIOLENCE = 1;
int sub_result = -1, sub_calls = 0, refund_calls = 0, saved = 0, waited = 0;
int refund_delta = 0;
#define GET_MONEY(ch) ((ch)->money)
#define GET_CHAR_SKILL(ch, index) ((ch)->only.pc->skills[index].learned)
#define GET_NAME(ch) "player"
#define MIN(a, b) ((a) < (b) ? (a) : (b))
void send_to_char(const char *, P_char) {}
bool epic_transaction_submit(P_char, int64_t delta, epic_reason_type, int64_t,
                             uint16_t, critical_source_site,
                             critical_deadline_class, void *, const void *, size_t) {
    ++refund_calls; refund_delta = delta; return true;
}
int SUB_MONEY(P_char, int, int) { ++sub_calls; return sub_result; }
int get_property(const char *, int fallback) { return fallback; }
bool do_save_silent(P_char, int) { ++saved; return true; }
void logit(int, const char *, ...) { assert(false); }
void CharWait(P_char, int) { ++waited; }
struct epic_skill_purchase_context {
    int skill, epic_cost, coins_cost, expected_skill;
};
'''

EPIC_CHECKS = r'''
int main() {
    pc_state pc;
    character ch;
    ch.only.pc = &pc;
    epic_command_result result;
    epic_skill_purchase_context context = {0, 5, 40, 20};
    const auto *bytes = reinterpret_cast<const uint8_t *>(&context);
    epic_skill_purchase_committed(&ch, true, result, 0, bytes, sizeof(context));
    assert(sub_calls == 1 && refund_calls == 1 && refund_delta == 5);
    assert(pc.skills[0].learned == 20 && saved == 0 && waited == 0);

    sub_result = 0;
    epic_skill_purchase_committed(&ch, true, result, 0, bytes, sizeof(context));
    assert(sub_calls == 2 && refund_calls == 1);
    assert(pc.skills[0].learned == 30 && pc.skills[0].taught == 30);
    assert(saved == 1 && waited == 1);
}
'''


class ActiveCashPurchaseRefusal(unittest.TestCase):
    def test_stat_shop_all_nine_purchases_refuse_before_debit_or_spell(self) -> None:
        source = PRELUDE + "\n" + function_body("src/world/epic.c", "int stat_shops(") + "\n" + CHECKS
        with tempfile.TemporaryDirectory(prefix="duris-stat-shop-") as directory:
            program = Path(directory) / "stat_shop.cpp"
            binary = Path(directory) / "stat_shop"
            program.write_text(source, encoding="utf-8")
            subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                            str(program), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

    def test_epic_skill_callback_refunds_rejected_coin_submission(self) -> None:
        source = EPIC_PRELUDE + "\n" + function_body(
            "src/classes/epic_skills.c", "void epic_skill_purchase_committed(") + "\n" + EPIC_CHECKS
        with tempfile.TemporaryDirectory(prefix="duris-epic-skill-") as directory:
            program = Path(directory) / "epic_skill.cpp"
            binary = Path(directory) / "epic_skill"
            program.write_text(source, encoding="utf-8")
            subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                            str(program), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

    def test_other_unsupported_costs_refuse_before_native_benefit(self) -> None:
        confirmation = function_body("src/cmd/interp.c", "void do_confirm(")
        guild = confirmation[confirmation.index('if (strstr(ch->desc->client_str, "found_asc"))'):]
        self.assertLess(guild.index("economic_gameplay_authority::active()"),
                        guild.index("found_asc(founder, ch"))
        self.assertLess(guild.index("economic_gameplay_authority::active()"),
                        guild.index("SUB_MONEY(ch, GUILD_COST"))
        self.assertLess(guild.index("ch->desc->confirm_state = CONFIRM_DONE;"),
                        guild.index("found_asc(founder, ch"))

        enhancement = function_body("src/item/enhance.c", "void do_enhance(")
        guard = enhancement.index("economic_gameplay_authority::active()")
        self.assertLess(enhancement.index("if (!argument || !*argument)"), guard)
        for mutation in ("perform_superior_enhancement(ch", "modenhance(ch", "enhance(ch"):
            self.assertLess(guard, enhancement.index(mutation))

        teacher = function_body("src/classes/epic_skills.c", "int epic_teacher(")
        self.assertLess(teacher.index("economic_gameplay_authority::active()"),
                        teacher.index("epic_transaction_submit(pl, -epics_cost"))
        completion = function_body("src/classes/epic_skills.c",
                                   "void epic_skill_purchase_committed(")
        self.assertLess(completion.index("if (SUB_MONEY(pl, context.coins_cost, 0) != 0)"),
                        completion.index("pl->only.pc->skills[context.skill].taught = learned"))
        self.assertIn("epic_reason_type::epic_skill_refund",
                      completion[completion.index("if (SUB_MONEY("):])

    def test_purchase_routes_remain_blocked_in_coverage_matrix(self) -> None:
        matrix = json.loads((ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json")
                            .read_text(encoding="utf-8"))
        routes = {route["id"]: route for route in matrix["routes"]}
        inventory = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json")
                               .read_text(encoding="utf-8"))
        writers = {row["id"]: row for row in inventory["writers"]}
        for route_id in ("epic.stat_shop_fee", "item.enhance_fee",
                         "guild.creation_fee", "epic.skill_purchase_cost"):
            with self.subTest(route=route_id):
                route = routes[route_id]
                self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
                self.assertFalse(route["current_critical_command_schema"]
                                 ["schema_2_gameplay_producer_connected"])
                self.assertIn("active", writers[route_id]["current_support"].lower())
                self.assertFalse(route["double_entry_evidence"]
                                 ["unified_operation_postings_observed"])


if __name__ == "__main__":
    unittest.main()
