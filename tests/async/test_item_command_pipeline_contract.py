#!/usr/bin/env python3
"""Contracts for the shared item command parser and ownership boundary."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from _paths import SRC


ROOT = Path(__file__).resolve().parents[2]
ACTOBJ = (SRC / "cmd/actobj.c").read_text(encoding="utf-8")
PARSER_H = (SRC / "item/item_command_parser.h").read_text(encoding="utf-8")
PARSER_C = (SRC / "item/item_command_parser.c").read_text(encoding="utf-8")
POLICY_H = (SRC / "item/item_command_policy.h").read_text(encoding="utf-8")
POLICY_C = (SRC / "item/item_command_policy.c").read_text(encoding="utf-8")
DOC = (ROOT / "docs/persistence/ITEM_COMMAND_PIPELINE.md").read_text(encoding="utf-8")


class ItemCommandPipelineContractTests(unittest.TestCase):
    def test_focused_parser_and_policy_are_build_wired(self):
        makefile = (SRC / "Makefile").read_text(encoding="utf-8")
        self.assertIn('#include "item/item_command_parser.h"', ACTOBJ)
        self.assertIn('#include "item/item_command_policy.h"', ACTOBJ)
        self.assertIn("item/item_command_parser.o", makefile)
        self.assertIn("item/item_command_policy.o", makefile)
        for token in (
            "item_get_command_kind",
            "item_get_command_parse",
            "item_command_uses_durable_ownership",
            "item_command_object_is_takeable",
            "item_command_container_is_valid",
            "item_command_resolve_put_destination",
            "item_command_resolve_drop_destination",
        ):
            self.assertIn(token, PARSER_H + PARSER_C + POLICY_H + POLICY_C)

    def test_get_has_one_bounded_parser_and_no_legacy_local_parser(self):
        start = ACTOBJ.index("void do_get(P_char ch")
        end = ACTOBJ.index("void do_put(P_char ch", start)
        do_get = ACTOBJ[start:end]
        self.assertEqual(do_get.count("item_get_command_parse(argument, &parsed)"), 1)
        self.assertNotIn("argument_interpreter(argument", do_get)
        self.assertNotIn('sscanf(arg1, "all.%s"', do_get)
        self.assertNotIn("do_get_mark_alldot", ACTOBJ)
        self.assertIn("type = static_cast<int>(parsed.kind)", do_get)
        self.assertIn("alldot = parsed.alldot", do_get)

    def test_every_item_path_uses_shared_durable_eligibility_and_resolvers(self):
        self.assertNotIn("static bool uses_generic_item_ownership", ACTOBJ)
        self.assertNotIn("static bool get_item_source_owner", ACTOBJ)
        self.assertNotIn("static bool do_get_obj_is_takeable", ACTOBJ)
        self.assertNotIn("static bool do_get_container_target_is_valid", ACTOBJ)
        self.assertGreaterEqual(
            ACTOBJ.count("item_command_uses_durable_ownership("), 10
        )
        self.assertGreaterEqual(
            ACTOBJ.count("item_get_source_owner("), 6
        )
        self.assertEqual(ACTOBJ.count("locker_owner_for_room("), 0)
        self.assertEqual(ACTOBJ.count("locker_owner_for_container("), 1)
        self.assertEqual(ACTOBJ.count("item_command_resolve_drop_destination("), 2)
        self.assertEqual(ACTOBJ.count("item_command_resolve_put_destination("), 2)

    def test_policy_has_explicit_boundaries_and_no_live_publication(self):
        self.assertIn("ITEM_TRANSIENT", POLICY_C)
        self.assertIn("item_custody_state::active", POLICY_C)
        self.assertIn("owner_is_virtual_source", (SRC / "item/item_get_policy.c").read_text())
        self.assertIn("ITEM_CORPSE", POLICY_C)
        self.assertIn("corpse", DOC)
        self.assertIn("item_transfer_reason::locker_deposit", POLICY_C)
        self.assertIn("item_transfer_reason::player_put", POLICY_C)
        self.assertIn("item_transfer_reason::player_drop", POLICY_C)
        self.assertNotIn("obj_to_obj(", POLICY_C)
        self.assertNotIn("obj_to_room(", POLICY_C)
        self.assertIn("item_movement_transaction_submit", ACTOBJ)
        self.assertIn("item_put_completion", ACTOBJ)
        self.assertIn("item_drop_completion", ACTOBJ)
        self.assertIn("completion callback publishes", DOC)

    def test_parser_runtime_forms(self):
        if not shutil.which("g++"):
            self.skipTest("native g++ is unavailable; GitHub Linux runs this contract")

        harness = r'''
#include "item/item_command_parser.h"

#include <cassert>
#include <cctype>
#include <cstring>
#include <string>

static bool is_fill_word(const char *word)
{
    for (const char *fill : {"in", "from", "with", "the", "on", "at", "to"})
        if (std::strcmp(word, fill) == 0)
            return true;
    return false;
}

void argument_interpreter(char *argument, char *first_arg, char *second_arg)
{
    char *outputs[] = {first_arg, second_arg};
    int output = 0;
    char *cursor = argument;
    while (output < 2)
    {
        while (*cursor == ' ')
            ++cursor;
        if (!*cursor)
        {
            outputs[output][0] = '\0';
            ++output;
            continue;
        }
        char word[MAX_INPUT_LENGTH] = {};
        size_t length = 0;
        while (cursor[length] > ' ' && length + 1 < sizeof(word))
        {
            word[length] = static_cast<char>(
                std::tolower(static_cast<unsigned char>(cursor[length])));
            ++length;
        }
        word[length] = '\0';
        cursor += length;
        if (is_fill_word(word))
            continue;
        std::strcpy(outputs[output], word);
        ++output;
    }
}

int strn_cmp(const char *left, const char *right, uint length)
{
    for (uint index = 0; index < length; ++index)
    {
        const unsigned char l = static_cast<unsigned char>(std::tolower(left[index]));
        const unsigned char r = static_cast<unsigned char>(std::tolower(right[index]));
        if (l != r || !l || !r)
            return l == r ? 0 : (l < r ? -1 : 1);
    }
    return 0;
}

static void expect(const char *input, item_get_command_kind kind, bool alldot,
                   const char *object, const char *container, const char *filter)
{
    item_get_command command = {};
    assert(item_get_command_parse(input, &command));
    assert(command.kind == kind);
    assert(command.alldot == alldot);
    assert(std::strcmp(command.object, object) == 0);
    assert(std::strcmp(command.container, container) == 0);
    assert(std::strcmp(command.filter, filter) == 0);
}

int main()
{
    expect("", item_get_command_kind::invalid, false, "", "", "");
    expect("ALL", item_get_command_kind::floor_all, false, "all", "", "");
    expect("all.sword", item_get_command_kind::floor_all, true, "all", "", "sword");
    expect("all from corpse", item_get_command_kind::all_from_container,
           false, "all", "corpse", "");
    expect("all.ring in bag", item_get_command_kind::all_from_container,
           true, "all", "bag", "ring");
    expect("sword from all", item_get_command_kind::item_from_all,
           false, "sword", "all", "");
    expect("ring in bag", item_get_command_kind::item_from_container,
           false, "ring", "bag", "");
    std::string too_long(MAX_INPUT_LENGTH, 'x');
    expect(too_long.c_str(), item_get_command_kind::invalid, false, "", "", "");
}
'''
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            source = directory / "parser_contract.cc"
            binary = directory / "parser_contract"
            source.write_text(harness, encoding="utf-8")
            subprocess.run(
                [
                    "g++",
                    "-std=c++20",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    f"-I{SRC}",
                    str(source),
                    str(SRC / "item/item_command_parser.c"),
                    "-o",
                    str(binary),
                ],
                check=True,
            )
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
