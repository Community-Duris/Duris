#!/usr/bin/env python3
"""Exercise the reconnect predicate without importing the manual SQL fixture."""
from pathlib import Path
import ast
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests/async/pa_item_creation_fixture.py"


class WindBladeVisibilityContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tree = ast.parse(FIXTURE.read_text(encoding="utf-8"), filename=str(FIXTURE))
        predicates = [
            node.args[0]
            for node in ast.walk(tree)
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id == "require"
            and len(node.args) == 2
            and isinstance(node.args[1], ast.Constant)
            and node.args[1].value == (
                "reconnected character could not see the still-unexpired Wind Blade"
            )
        ]
        if len(predicates) != 1:
            raise AssertionError("expected one real reconnect visibility predicate")
        predicate = predicates[0]
        if not isinstance(predicate, ast.Compare) or not isinstance(predicate.left, ast.Constant):
            raise AssertionError("expected a literal short-description membership check")
        cls.predicate = predicate
        cls.literal = predicate.left.value
        cls.expression = compile(ast.Expression(cls.predicate), str(FIXTURE), "eval")
        vnum = next(
            ast.literal_eval(node.value)
            for node in tree.body
            if isinstance(node, ast.Assign)
            and any(isinstance(target, ast.Name) and target.id == "WIND_BLADE_VNUM"
                    for target in node.targets)
        )
        lines = (ROOT / "areas_mini/mini.obj").read_text(encoding="utf-8").splitlines()
        raw_description = lines[lines.index(f"#{vnum}") + 2].removesuffix("~")
        cls.description = re.sub(r"&(?:[+=].|.)", "", raw_description)

    def visible(self, inventory, equipment):
        return eval(self.expression, {"__builtins__": {}},
                    {"inventory": inventory, "equipment": equipment})

    def test_predicate_is_bound_to_the_mini_world_short_description(self):
        self.assertIsInstance(self.predicate, ast.Compare)
        self.assertIsInstance(self.predicate.left, ast.Constant)
        self.assertEqual(self.literal, self.description.lower())

    def test_reconnected_equipment_render_is_accepted(self):
        equipment = f"You are using:\n<primary weapon>     {self.description} (glowing)\n"
        self.assertTrue(self.visible("You are carrying:\na small bandage\n", equipment))

    def test_carried_weapon_render_is_accepted(self):
        self.assertTrue(self.visible(f"You are carrying:\n{self.description}\n", ""))

    def test_case_variation_is_accepted(self):
        self.assertTrue(self.visible("", self.description.upper()))

    def test_unrelated_blade_and_missing_weapon_are_rejected(self):
        self.assertFalse(self.visible("a mundane training blade", ""))
        self.assertFalse(self.visible("You are carrying:\nNothing.", "You are using:\nNothing."))


if __name__ == "__main__":
    unittest.main()
