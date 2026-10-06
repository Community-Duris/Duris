#!/usr/bin/env python3
"""Source contract for the private SQL wallet-root admission projection."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "src/economy/economic_gameplay_authority.h"
IMPLEMENTATION = ROOT / "src/economy/economic_gameplay_authority.c"


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    # A default argument may contain braces before the actual function body.
    parameters = source.index("(", start)
    parameter_depth = 0
    for offset in range(parameters, len(source)):
        if source[offset] == "(":
            parameter_depth += 1
        elif source[offset] == ")":
            parameter_depth -= 1
            if parameter_depth == 0:
                break
    else:
        raise AssertionError(f"unterminated parameters: {signature}")
    opening = source.index("{", offset + 1)
    depth = 0
    for offset in range(opening, len(source)):
        if source[offset] == "{":
            depth += 1
        elif source[offset] == "}":
            depth -= 1
            if depth == 0:
                return source[opening : offset + 1]
    raise AssertionError(f"unterminated function: {signature}")


class FunctionBodyParsingContract(unittest.TestCase):
    def test_default_initializer_precedes_actual_immutable_publication_body(self):
        body = "{ next->scope_version = 1; current.store(next); }"
        source = "void publish_projection(std::shared_ptr<const projection> expected = {})\n" + body
        self.assertEqual(function_body(source, "publish_projection("), body)

    def test_nested_parameter_call_precedes_actual_compare_exchange_body(self):
        body = "{ if (expected) { current.compare_exchange_strong(expected, next); } }"
        source = "void publish_projection(int value = choose(1), projection expected = {}) noexcept\n" + body
        self.assertEqual(function_body(source, "publish_projection("), body)


class SqlWalletRootProjectionContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.implementation = IMPLEMENTATION.read_text(encoding="utf-8")

    def test_installer_is_private_and_sql_owner_restricted(self):
        self.assertLess(self.header.index("private:"),
                        self.header.index("install_sql_wallet_root_qualification"))
        self.assertIn("sql_wallet_root_qualification_install_key", self.header)
        key_definition = self.header.split(
            "class sql_wallet_root_qualification_install_key", 1
        )[1].split("\n\t};", 1)[0]
        self.assertIn("friend class economic_sql_accounting_lifecycle_transaction;",
                      key_definition)
        self.assertIn("friend class economic_gameplay_authority_test_access;",
                      key_definition)
        self.assertNotIn("flatfile_accounting_lifecycle_transaction", key_definition)
        self.assertIn("static economic_accounting_error install_sql_wallet_root_qualification(",
                      self.header)

    def test_projection_scope_is_fixed_and_published_immutably(self):
        self.assertIn("projection_scope::sql_wallet_root_qualification", self.implementation)
        self.assertIn("SQL_WALLET_ROOT_QUALIFICATION_VERSION = 1", self.implementation)
        self.assertIn("uint16_t scope_version = 0;", self.implementation)
        self.assertIn("std::atomic<std::shared_ptr<const admission_projection>> current;",
                      self.implementation)
        publish = function_body(self.implementation, "publish_projection(")
        self.assertLess(publish.index("next->scope_version ="), publish.index("current.store("))
        if "current.compare_exchange_strong(" in publish:
            self.assertLess(publish.index("next->scope_version ="),
                            publish.index("current.compare_exchange_strong("))

    def test_qualification_precedes_all_frozen_schema_two_returns(self):
        currency = function_body(
            self.implementation,
            "economic_accounting_error economic_gameplay_authority::prepare_currency(",
        )
        coin = function_body(
            self.implementation,
            "economic_gameplay_authority::prepare_coin_transfer(",
        )
        item = function_body(
            self.implementation,
            "economic_gameplay_authority::prepare_item_transfer(",
        )
        schema_two = r"command->schema_version\s*==\s*CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION"
        for body in (currency, coin, item):
            match = re.search(schema_two, body)
            self.assertIsNotNone(match)
            if match is None:
                raise AssertionError("schema-2 branch is missing")
            self.assertLess(body.index("sql_wallet_root_scope(*selected)"), match.start())
        self.assertIn("return error::unauthorized;", currency)
        self.assertIn("*selected, *command, &expected_intent", coin)
        self.assertIn("expected_intent == command->accounting_intent", coin)

    def test_real_coin_transfer_contract_helper_is_reused(self):
        helper = function_body(
            self.implementation,
            "economic_accounting_error qualified_wallet_root_intent(",
        )
        self.assertIn("command.type != critical_command_type::coin_transfer", helper)
        self.assertIn("coin_transfer_command_decode_payload(command, &payload)", helper)
        self.assertIn("payload.source.change.type != critical_command_type::account_bank", helper)
        self.assertIn("coin_transfer_accounting_intent(admission, selected.epoch", helper)

    def test_production_has_no_qualification_installer_callers(self):
        name = "install_sql_wallet_root_qualification"
        allowed = {HEADER.resolve(), IMPLEMENTATION.resolve()}
        callers = []
        for path in (ROOT / "src").rglob("*"):
            if path.suffix not in {".c", ".h"} or path.resolve() in allowed:
                continue
            if name in path.read_text(encoding="utf-8"):
                callers.append(path.relative_to(ROOT).as_posix())
        self.assertEqual(callers, [])


if __name__ == "__main__":
    unittest.main()
