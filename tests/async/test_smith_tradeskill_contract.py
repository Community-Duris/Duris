#!/usr/bin/env python3
"""Paid repair/smith source contracts; no C++ or native atomicity execution.

Legacy hazards are comparison facts, not permanent behavior requirements.
Intentional primary fixes require explicit contract updates and original review.
Use --source-root to check a frozen public export without changing its files.
"""

import argparse
import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]


SOURCE_PATHS = (
    "src/economy/tradeskill.c",
    "src/economy/shop.c",
    "src/item/item_movement_transaction.c",
)
LEXER = re.compile(
    r'(?P<skip>\s+|//[^\n]*|/\*.*?\*/)|'
    r'(?P<literal>"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|'
    r'(?P<token>[A-Za-z_]\w*|\d+|<<=|>>=|<=>|->\*|\.\.\.|::|->|\.\*|'
    r'<<|>>|\+=|-=|\*=|/=|%=|\^=|&=|\|=|&&|\|\||==|!=|<=|>=|\+\+|--|##|\S)',
    re.DOTALL,
)


class SourceContractError(AssertionError):
    pass


def tokenize(source):
    """Ignore whitespace/comments; keep quoted braces out of block matching.

    This lexer covers the selected public C++ functions, not the C++ grammar.
    Raw string syntax is outside this source scope and fails explicitly.
    """
    result = []
    for match in LEXER.finditer(source):
        if match.lastgroup != "skip":
            if result and result[-1] in ("R", "u8R", "uR", "UR", "LR") and match.lastgroup == "literal":
                raise SourceContractError("raw string literal outside selected lexer scope")
            result.append(match.group())
    return tuple(result)


def closing(tokens, start, opening, ending, scope):
    depth = 0
    for index in range(start, len(tokens)):
        if tokens[index] == opening:
            depth += 1
        elif tokens[index] == ending:
            depth -= 1
            if depth == 0:
                return index
    raise SourceContractError(f"{scope}: unclosed {opening}")


class Region:
    def __init__(self, tokens, scope):
        self.tokens = tokens
        self.scope = scope

    def positions(self, fragment):
        wanted = tokenize(fragment)
        return [index for index in range(len(self.tokens) - len(wanted) + 1)
                if self.tokens[index:index + len(wanted)] == wanted]

    def one(self, fragment, label):
        found = self.positions(fragment)
        if len(found) != 1:
            raise SourceContractError(
                f"{self.scope}: {label}: expected one {fragment!r}, found {len(found)}")
        return found[0]

    def ordered(self, label, *fragments):
        positions = [self.one(fragment, label) for fragment in fragments]
        if positions != sorted(set(positions)):
            raise SourceContractError(f"{self.scope}: {label}: source order changed")

    def block(self, prefix, label):
        start = self.one(prefix, label) + len(tokenize(prefix))
        if start >= len(self.tokens) or self.tokens[start] != "{":
            raise SourceContractError(f"{self.scope}: {label}: expected braced body")
        end = closing(self.tokens, start, "{", "}", self.scope)
        return Region(self.tokens[start + 1:end], self.scope + "/" + label)


def function(source_tokens, name, path):
    """Require one complete definition; declarations/calls are not definitions."""
    definitions = []
    for index, token in enumerate(source_tokens[:-1]):
        if token != name or source_tokens[index + 1] != "(":
            continue
        end_args = closing(source_tokens, index + 1, "(", ")", path + ":" + name)
        start_body = end_args + 1
        if start_body < len(source_tokens) and source_tokens[start_body] == "{":
            end_body = closing(source_tokens, start_body, "{", "}", path + ":" + name)
            definitions.append(Region(source_tokens[index:end_body + 1], path + ":" + name))
    if len(definitions) != 1:
        raise SourceContractError(f"{path}:{name}: expected one complete definition, found {len(definitions)}")
    return definitions[0]


def validate_sources(sources):
    """Check only selected source predicates; return their labels for review."""
    lexed = {path: tokenize(sources[path]) for path in SOURCE_PATHS}
    trade, shop, movement = SOURCE_PATHS
    smith = function(lexed[trade], "smith", trade)
    grant = function(lexed[trade], "grant_tradeskill_item", trade)
    repair = function(lexed[shop], "shopping_repair", shop)
    keeper = function(lexed[shop], "shop_keeper", shop)
    refuse = function(lexed[shop], "refuse_unported_shop_mutation", shop)
    gem = function(lexed[shop], "accept_gem_for_debt", shop)
    transact = function(lexed[shop], "transact", shop)
    submit = function(lexed[movement], "item_creation_grant_submit_to_player", movement)
    queue = function(lexed[movement], "queue_creation_grant", movement)
    passed = []

    def ordered(region, label, *fragments):
        region.ordered(label, *fragments)
        passed.append(label)

    def require(region, label, condition):
        if not condition:
            raise SourceContractError(f"{region.scope}: {label}: predicate changed")
        passed.append(label)

    # Preserve the original debit < creation < grant < retirement assertion.
    # Earlier obj_from_char is detachment, distinct from extract_obj retirement.
    ordered(smith, "smith.debit-create-grant-retire",
            "SUB_MONEY(pl, price, 0)", "forge_create(choice, pl, needed_ore[0]->material)",
            "grant_tradeskill_item(pl, tobj)", "extract_obj(needed_ore[j], TRUE)")
    require(smith, "smith.refund-calls", len(smith.positions("ADD_MONEY(pl, price);")) == 2)
    for label, condition in (
        ("smith.creation-refusal-refund", "if (!(tobj = forge_create(choice, pl, needed_ore[0]->material)))"),
        ("smith.grant-refusal-refund", "if (!grant_tradeskill_item(pl, tobj))"),
    ):
        failure = smith.block(condition, label)
        ordered(failure, label, "ADD_MONEY(pl, price);", "obj_to_char(needed_ore[j], pl);", "return TRUE;")

    ordered(refuse, "shop.active-service-refusal",
            "if (!economic_gameplay_authority::active()) return false;",
            "if (money_trade && shop_trade_preparation_owner::production_available()) return false;",
            "return true;")
    ordered(keeper, "shop.service-guard-before-smith",
            "if ((cmd == CMD_BUY || cmd == CMD_SELL || cmd == CMD_PERUSE || cmd == CMD_REPAIR || cmd == CMD_FORGE) && refuse_unported_shop_mutation(ch, cmd == CMD_BUY || cmd == CMD_SELL)) return TRUE;",
            "if (cmd == CMD_FORGE) return smith(keeper, ch, cmd, arg);")
    ordered(repair, "repair.direct-guard-before-selection",
            "if (refuse_unported_shop_mutation(ch)) return;", "is_ok(keeper, ch, shop_nr)",
            "get_selling_obj(ch, argm, keeper, shop_nr, TRUE, TRUE)")
    ordered(smith, "smith.direct-guard-before-selection",
            "if (economic_gameplay_authority::active())", "j = GET_VNUM(ch);",
            "for (tobj = ch->carrying; tobj; tobj = tobj->next_content)")
    active = smith.block("if (economic_gameplay_authority::active())", "smith.direct-guard-return")
    require(active, "smith.direct-guard-return", len(active.positions("return TRUE;")) == 1)
    ordered(transact, "transact.direct-active-refusal",
            "if (refuse_unported_shop_mutation(from)) return FALSE;",
            "if (from->in_room == to->in_room)")

    require(smith, "smith.keeper-customer-parameters", bool(smith.positions(
        "smith(P_char ch, P_char pl, int cmd, char *arg)")))
    ordered(smith, "smith.legacy-menu-bound",
            "j = GET_VNUM(ch);", "for (i = 0; smith_array[i].vnum > 0; i++)",
            "choice = atoi(arg); if (choice < 1 || choice > i) return FALSE;",
            "choice = sdata->items[choice - 1];")
    ordered(smith, "smith.keeper-detach-before-debit",
            "for (tobj = ch->carrying; tobj; tobj = tobj->next_content)",
            "obj_from_char(tobj);", "SUB_MONEY(pl, price, 0)")
    require(smith, "smith.customer-ore-return-calls",
            len(smith.positions("obj_to_char(needed_ore[j], pl);")) == 6 and
            len(smith.positions("obj_to_char(")) == 6)

    ordered(repair, "repair.prepayment-weapon-and-condition-order",
            "wpn = read_object(obj_index[obj->R_num].virtual_number, VIRTUAL);",
            "obj->value[3] = wpn->value[3];", "obj->value[3] = MSG_SLASH;",
            "extract_obj(wpn);", "if (obj->condition > 0)", "if (obj->condition < 100) {",
            "transact(ch, gem, keeper, cost)")
    ordered(gem, "gem.selector-disabled", "return NULL; obj = ch->carrying;")
    ordered(transact, "gem.exchange-disabled", "merchandise = 0;", "if (merchandise)")

    require(grant, "grant.smith-no-business-callback", bool(grant.positions(
        "item_creation_grant_submit_to_player(ch, object, ch, NULL, economic_source_kind::crafting)")) and
        not grant.positions("item_creation_grant_submit_to_player_with_completion("))
    require(grant, "grant.refused-output-cleanup", bool(grant.positions("if (object) extract_obj(object, FALSE);")))
    require(submit, "grant.queue-without-business-callback", bool(submit.positions(
        "return queue_creation_grant(actor, object, recipient, NOWHERE, target_container, false, false, nullptr, nullptr, 0, nullptr, source, source_id);")))
    ordered(queue, "grant.accepted-is-not-completed",
            "if (queue.batch_submission || queue.active || queue.requests.size() > 1) return true;",
            "if (start_creation_grant(actor, queue, &reject)) return true;",
            "if (item_movement_reject_is_transient(reject)) return true;",
            "queue.requests.pop_back();")
    return tuple(passed)


class SmithTradeskillSourceContract(unittest.TestCase):
    def test_paid_service_source_order_and_legacy_comparison_facts(self):
        sources = {path: (ROOT / path).read_text(encoding="utf-8") for path in SOURCE_PATHS}
        self.assertTrue(validate_sources(sources))

    def test_writers_inventory_routes_smith_as_crafting_cost(self):
        writers_path = ROOT / "docs/persistence/economy_accounting/writers.json"
        writers = json.loads(writers_path.read_text(encoding="utf-8"))
        entry = next((w for w in writers["writers"] if w["id"] == "crafting.smith"), None)
        self.assertIsNotNone(entry)
        self.assertEqual(entry["reason"], "crafting_cost")
        self.assertEqual(entry["symbol"], "smith")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    options, remaining = parser.parse_known_args()
    ROOT = options.source_root.resolve()
    program = unittest.main(argv=[sys.argv[0], *remaining], exit=False)
    print(json.dumps({"source_contract_only": True,
                      "passed": program.result.wasSuccessful(),
                      "native_compound_qualification": False}))
    sys.exit(0 if program.result.wasSuccessful() else 1)
