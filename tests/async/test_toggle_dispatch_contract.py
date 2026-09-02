#!/usr/bin/env python3
"""Keep toggle names, messages, and post-kingdom dispatch indexes aligned."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ACTOTH = ROOT / "src/cmd/actoth.c"


class ToggleDispatchContractTest(unittest.TestCase):
    """Protect the source-level contract between toggle names and dispatch cases."""

    @classmethod
    def setUpClass(cls) -> None:
        """Extract toggle names, messages, and switch cases from the source."""
        cls.source = ACTOTH.read_text()
        names_body = cls.source.split("static const char *toggles_list[] = {", 1)[1].split(
            "};", 1
        )[0]
        cls.names = re.findall(r'^\s*"([^"\\]*(?:\\.[^"\\]*)*)"', names_body, re.MULTILINE)

        messages_body = cls.source.split("static const char *tog_messages[][2] = {", 1)[
            1
        ].split("};", 1)[0]
        cls.message_count = len(re.findall(r'^\s*\{\s*"', messages_body, re.MULTILINE))

        switch_body = cls.source.split("switch (tog_nr)", 1)[1].split("\n\tdefault:", 1)[0]
        cls.cases = {
            int(number): body
            for number, body in re.findall(
                r"\n\tcase (\d+):(.*?)(?=\n\tcase \d+:|\Z)", switch_body, re.DOTALL
            )
        }

    def test_parallel_arrays_stay_aligned(self) -> None:
        """Require one reachable single-token name for every toggle message."""
        self.assertEqual(self.names[-1], r"\n")
        self.assertEqual(self.message_count, len(self.names) - 1)
        self.assertNotIn("kingdom", self.names)
        self.assertFalse(
            [
                name
                for name in self.names[:-1]
                if any(character.isspace() for character in name)
            ],
            "toggle names must be reachable by the first-token dispatcher",
        )

    def test_multiword_toggle_spellings_and_values(self) -> None:
        """Verify canonical names and compatibility aliases preserve values."""
        expected = {
            29: "nolocate",
            48: "groupneeded",
            60: "nolevel",
        }
        for index, name in expected.items():
            with self.subTest(name=name):
                self.assertEqual(self.names[index], name)
                for value in ("", "on", "off"):
                    arguments = f"{name} {value}".split()
                    self.assertEqual(arguments[0], self.names[index])
                    self.assertEqual(arguments[1:], [value] if value else [])

        parser = self.source.split("arg = one_argument(arg, Gbuf1);", 1)[1].split(
            "number = atoi(arg);", 1
        )[0]
        for alias, canonical in {
            "no-locate": "nolocate",
            "group-needed": "groupneeded",
            "no-level": "nolevel",
        }.items():
            self.assertIn(f'!str_cmp(Gbuf1, "{alias}")', parser)
            self.assertIn(f'strcpy(Gbuf1, "{canonical}");', parser)
        for second_word, canonical in {
            "locate": "nolocate",
            "level": "nolevel",
            "needed": "groupneeded",
        }.items():
            self.assertIn(f'!str_cmp(Gbuf3, "{second_word}")', parser)
            self.assertIn(f'strcpy(Gbuf1, "{canonical}");', parser)
        self.assertEqual(parser.count("arg = value;"), 3)

    def test_post_retirement_dispatch_indexes(self) -> None:
        """Keep active dispatch indexes stable after retired toggle slots."""
        expected = {
            32: ("shipmap", "PLR2_SHIPMAP"),
            33: ("take", "PLR2_NOTAKE"),
            34: ("terse", "PLR2_TERSE"),
            35: ("quickchant", "PLR2_QUICKCHANT"),
            36: ("rwc", "PLR2_RWC"),
            37: ("project", "PLR2_PROJECT"),
            38: ("zzxyzz", "PLR2_NPC_HOG"),
            39: ("afk", "PLR_AFK"),
            40: ("nchat", "PLR2_NCHAT"),
            41: ("damage", "PLR2_DAMAGE"),
            47: ("heal", "PLR2_HEAL"),
            48: ("groupneeded", "PLR2_LGROUP"),
            49: ("experience", "PLR2_EXP"),
            50: ("showspec", "PLR2_SPEC"),
            51: ("hint", "PLR2_HINT_CHANNEL"),
            52: ("webinfo", "PLR2_WEBINFO"),
            53: ("acc", "PLR2_ACC"),
            54: ("quest", "PLR2_SHOW_QUEST"),
            55: ("boon", "PLR2_BOON"),
            56: ("newbie", "PLR2_NEWBIEEQ"),
            57: ("beep", "PLR3_NOBEEP"),
            58: ("underline", "PLR3_UNDERLINE"),
            59: ("surname", "PLR3_SURNAMES"),
            60: ("nolevel", "PLR3_NOLEVEL"),
            61: ("epic", "PLR3_EPICWATCH"),
            62: ("petdamage", "PLR3_PET_DAMAGE"),
            63: ("guildname", "PLR3_GUILDNAME"),
            64: ("gmcp", "PLR3_NOGMCP"),
            65: ("jchat", "PLR3_JESTROS"),
        }
        self.assertEqual({index: self.names[index] for index in expected}, {
            index: name for index, (name, _) in expected.items()
        })
        for index, (_, flag) in expected.items():
            self.assertIn(flag, self.cases[index], f"toggle index {index}")

        for inactive_index in range(42, 47):
            self.assertNotIn(inactive_index, self.cases)


if __name__ == "__main__":
    unittest.main()
