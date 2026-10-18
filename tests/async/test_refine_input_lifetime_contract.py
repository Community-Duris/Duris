#!/usr/bin/env python3
"""Refinement must not read pooled input metadata after native extraction.

This checks the original producer's lifetime boundary without replacing its
object pool, extraction, RNG or other native providers. Gameplay execution is
a separate check. DURIS_REFINE_SOURCE permits the same checks on a pinned
historical source when collecting a regression's RED/GREEN evidence.
"""

import os
from pathlib import Path
import re
import unittest

from contract_text import code_text


ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get("DURIS_REFINE_SOURCE", ROOT / "src/economy/tradeskill.c"))


class RefineInputLifetimeContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = SOURCE.read_text(encoding="utf-8")
        start = source.index("void do_refine(")
        end = source.index("\nvoid do_dice(", start)
        body = code_text(source[start:end])
        loop = re.search(
            r"for\s*\(\s*t_obj\s*=\s*ch->carrying\s*;\s*t_obj\s*;"
            r"\s*t_obj\s*=\s*nextobj\s*\)", body)
        if loop is None:
            raise AssertionError("The original refine consumption loop is missing")
        opening = body.index("{", loop.end())
        depth = 1
        closing = opening + 1
        while depth and closing < len(body):
            depth += (body[closing] == "{") - (body[closing] == "}")
            closing += 1
        if depth:
            raise AssertionError("The refine consumption loop is incomplete")
        cls.loop = body[opening:closing]
        cls.extractions = list(re.finditer(r"\bextract_obj\s*\(\s*t_obj\s*,", cls.loop))
        if len(cls.extractions) != 2:
            raise AssertionError("Expected original material and ore extraction sites")

    def test_input_vnum_is_not_read_after_extraction(self):
        first_extraction = self.extractions[0].start()
        reads = list(re.finditer(r"\bOBJ_VNUM\s*\(\s*t_obj\s*\)", self.loop))
        self.assertTrue(reads, "Refinement must observe the genuine input vnum")
        self.assertTrue(all(read.start() < first_extraction for read in reads),
                        "Input vnum was read after the pooled object was extracted")

    def test_ore_chance_uses_a_value_frozen_while_owned(self):
        first_extraction = self.extractions[0].start()
        captures = list(re.finditer(
            r"\b(?:const\s+)?(?:int|auto)\s+(\w+)\s*=\s*"
            r"OBJ_VNUM\s*\(\s*t_obj\s*\)\s*;", self.loop))
        frozen_names = [capture.group(1) for capture in captures
                        if capture.start() < first_extraction]
        chance = re.search(r"\borechance\s*=\s*([^;]+);", self.loop)
        self.assertIsNotNone(chance, "The original ore chance contribution is missing")
        self.assertLess(chance.start(), self.extractions[1].start(),
                        "Ore chance was derived after ore extraction")
        self.assertTrue(any(re.search(r"\b" + re.escape(name) + r"\b", chance.group(1))
                            for name in frozen_names),
                        "Ore chance does not use an owned input-vnum capture")


if __name__ == "__main__":
    unittest.main()
