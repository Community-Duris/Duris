#!/usr/bin/env python3
"""Exercise the paid mail admission gate and pin other service purchase gates."""
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
#include <cstdlib>
#include <cstring>
#include <string>

constexpr int MIN_MAIL_LEVEL = 1;
constexpr int STAMP_PRICE = 10;
constexpr int MAX_MAIL_SIZE = 4096;
constexpr int TO_ROOM = 1;
constexpr int PLR_MAIL = 1;
constexpr int MEM_TAG_BUFFER = 0;
constexpr int TRUE = 1;
struct descriptor { char *name = nullptr; char **str = nullptr; int max_str = 0; };
struct character {
    int level = 10, money = 20;
    bool trusted = false;
    struct { int act = 0; } specials;
    descriptor *desc = nullptr;
};
using P_char = character *;
bool active_mode = false;
int debit_calls = 0, act_calls = 0;
std::string last_message;
class economic_gameplay_authority {
public:
    static bool active() { return active_mode; }
};
#define GET_LEVEL(ch) ((ch)->level)
#define GET_MONEY(ch) ((ch)->money)
#define IS_TRUSTED(ch) ((ch)->trusted)
#define SET_BIT(target, mask) ((target) |= (mask))
#define FREE(target) do { std::free(target); (target) = nullptr; } while (0)
#define CREATE(target, type, count, tag) ((target) = static_cast<type *>(std::malloc(sizeof(type) * (count))))
void send_to_char(const char *message, P_char) { last_message = message; }
const char *coin_stringv(int) { return "ten coins"; }
void act(const char *, int, P_char, int, int, int) { ++act_calls; }
int SUB_MONEY(P_char ch, int cost, int) { ++debit_calls; ch->money -= cost; return 0; }
char *str_dup(const char *value) {
    const size_t size = std::strlen(value) + 1;
    auto *copy = static_cast<char *>(std::malloc(size));
    std::memcpy(copy, value, size);
    return copy;
}
'''

CHECKS = r'''
int main() {
    descriptor desc;
    character ch, mailman;
    ch.desc = &desc;
    char recipient[] = "recipient";

    active_mode = true;
    postmaster_send_mail(&ch, &mailman, recipient);
    assert(last_message.find("unavailable") != std::string::npos);
    assert(debit_calls == 0 && act_calls == 0 && ch.money == 20);
    assert(ch.specials.act == 0 && !desc.name && !desc.str);

    active_mode = false;
    postmaster_send_mail(&ch, &mailman, recipient);
    assert(debit_calls == 1 && act_calls == 1 && ch.money == 10);
    assert(ch.specials.act & PLR_MAIL);
    assert(desc.name && !std::strcmp(desc.name, recipient));
    assert(desc.str && *desc.str == nullptr && desc.max_str == MAX_MAIL_SIZE);
    FREE(desc.name);
    FREE(desc.str);
}
'''


class ActiveServicePurchaseRefusal(unittest.TestCase):
    def test_paid_mail_refuses_before_debit_or_message_edit(self) -> None:
        source = PRELUDE + "\n" + function_body(
            "src/cmd/mail.c", "void postmaster_send_mail(") + "\n" + CHECKS
        with tempfile.TemporaryDirectory(prefix="duris-mail-cost-") as directory:
            program = Path(directory) / "mail_cost.cpp"
            binary = Path(directory) / "mail_cost"
            program.write_text(source, encoding="utf-8")
            subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                            str(program), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

    def test_remort_and_tickets_refuse_before_purchase_effects(self) -> None:
        remort = function_body("src/specs/specs.mobile.c", "int monk_remort(")
        guard = remort.index("economic_gameplay_authority::active()")
        for mutation in ("SUB_MONEY(pl, plat", "forget_spells(pl", "pl->player.m_class = CLASS_MONK"):
            self.assertLess(guard, remort.index(mutation))

        flight = function_body("src/world/transport.c", "bool flying_transport_cmd_buy(")
        guard = flight.index("economic_gameplay_authority::active()")
        for mutation in ("SUB_MONEY(ch, cost", "read_object(VNUM_OBJ_FLIGHT_PATH_TICKET", "obj_to_char(ticket, ch)"):
            self.assertLess(guard, flight.index(mutation))

        ferry = function_body("src/world/ferryact.c", "int ferry_automat_proc(")
        guard = ferry.index("economic_gameplay_authority::active()")
        for mutation in ("SUB_MONEY(ch, ticket_cost", "read_object(FERRY_TICKET_VNUM", "obj_to_char(ticket, ch)"):
            self.assertLess(guard, ferry.index(mutation))

    def test_registry_does_not_report_these_routes_as_supported(self) -> None:
        matrix = json.loads((ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json")
                            .read_text(encoding="utf-8"))
        routes = {route["id"]: route for route in matrix["routes"]}
        for route_id in ("mail.postage", "special.monk_remort_fee",
                         "travel.flying_ticket", "travel.ferry_ticket"):
            with self.subTest(route=route_id):
                route = routes[route_id]
                self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
                self.assertFalse(route["current_critical_command_schema"]
                                 ["schema_2_gameplay_producer_connected"])
                self.assertFalse(route["double_entry_evidence"]
                                 ["unified_operation_postings_observed"])


if __name__ == "__main__":
    unittest.main()
