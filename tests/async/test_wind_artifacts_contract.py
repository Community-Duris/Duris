#!/usr/bin/env python3
"""Regression contracts for Dagger of Wind and Sphere of Wind artifact procs.

Verifies:
1. Dagger of Wind:
   - Pet NPCs and charmed NPCs are blocked from triggering escape teleport.
   - Non-pet NPCs trigger at 1/4 the normal rate (1 in 60 vs 1 in 15 for players).
2. Sphere of Wind / Lightning:
   - Rub activation validates alive state, worn location, name matching, and safe room.
   - Rub whirlwind pulse respects artifact hostility bounds.
   - Rub activation returns TRUE instead of falling through.
   - Melee hit proc remains reachable.
"""

from pathlib import Path
import re
import unittest

from contract_text import code_text


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src/specs/specs.underworld.c"


class WindArtifactsContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")

    def test_dagger_of_wind_pet_and_npc_scaling(self):
        start = self.source.index("int dagger_of_wind(")
        end = self.source.index("\nint ", start + 1)
        body = code_text(self.source[start:end])

        # Pet and charm check
        self.assertIn("IS_NPC(ch)", body)
        self.assertIn("IS_PC_PET(ch)", body)
        self.assertIn("AFF_CHARM", body)

        # 4x reduced chance for non-pet NPC (1 in 60 vs 1 in 15)
        chance_match = re.search(r"const\s+int\s+chance\s*=\s*IS_NPC\s*\(\s*ch\s*\)\s*\?\s*59\s*:\s*14\s*;", body)
        self.assertIsNotNone(chance_match, "Dagger of Wind must use 59 for NPCs (1-in-60) and 14 for PCs (1-in-15)")
        self.assertIn("!number(0, chance)", body)

    def test_sphere_of_wind_rub_and_melee_proc_reachability(self):
        start = self.source.index("int lightning(")
        end = self.source.index("\nint ", start + 1)
        body = code_text(self.source[start:end])

        # Rub activation requirements
        self.assertIn("OBJ_WORN_BY(obj, ch)", body)
        self.assertIn("isname(arg, obj->name)", body)
        self.assertIn("is_in_safe(ch)", body)
        self.assertIn("native_artifact_hostile(ch, vict)", body)

        # Must return TRUE from successful rub
        rub_block = re.search(r"if\s*\(cmd\s*==\s*CMD_RUB.*?\n\t\{(?P<content>.*?)\n\t\}", body, re.DOTALL)
        self.assertIsNotNone(rub_block, "Sphere of Wind rub block must exist")
        self.assertIn("return TRUE;", rub_block.group("content"))

        # Melee proc must be reachable
        after_rub = body[rub_block.end():]
        self.assertNotIn("return FALSE;", after_rub[:after_rub.index("CMD_MELEE_HIT")],
                         "Unconditional return must not precede CMD_MELEE_HIT proc")
        self.assertIn("cmd != CMD_MELEE_HIT", after_rub)


if __name__ == "__main__":
    unittest.main()
